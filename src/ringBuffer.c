/*****************************************************************************
 * @file    ringBuffer.c
 * @author  Kshitij Mistry
 * @brief
 * @todo Add support for self allocated buffer memory, user will provide the memory pointer
 *       and size while creating the buffer instance.
 *****************************************************************************/

/*****************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "ringBuffer.h"
#include <stdlib.h>
#include <string.h>
#include "common_def.h"

/*****************************************************************************
 * MACROS
 *****************************************************************************/
#define _BYTES_PER_MEGA_BYTE             (1000000LL)

#define MAX_ALLOWED_BUFFER_SIZE_IN_BYTES (10 * _BYTES_PER_MEGA_BYTE)  // 10 Mega Bytes (TODO : We can make this configurable)

#define INVALID_BUFFER_HANDLE            (-1)  // Invalid buffer handle

#define MAX_BUFFER_HANDLE                (10)  // (TODO : We can make this configurable)

#define IS_VALID_BUFFER_HANDLE(handle) \
    (((handle) >= 0) && ((handle) < MAX_BUFFER_HANDLE) && (gRbInfo[(handle)].bufferHandle != INVALID_BUFFER_HANDLE))

/*****************************************************************************
 * VARIABLES
 *****************************************************************************/
Rb_Info_t gRbInfo[MAX_BUFFER_HANDLE] = {0}; /**< Ring buffer information for each user */

/*****************************************************************************
 * FUNCTION DECLARATIONS
 *****************************************************************************/
static cBool handleFragmentedRead(Rb_Info_t *rbInfo, cU8_t **readPtr, cU64_t *dataBytes);

static cBool isFreeDataIndexAvailable(cI32_t bufferHandle);

static cU64_t getContiguousFreeSpace(cI32_t bufferHandle);

static cU64_t getFreeSpace(cI32_t bufferHandle);

static cU64_t getOccupiedSpace(cI32_t bufferHandle);

/*****************************************************************************
 * FUNCTION DEFINATIONS
 *****************************************************************************/
//----------------------------------------------------------------------------
/**
 * @brief Initialize buffer module
 */
void Rb_InitModule(void)
{
    cU8_t handleId = 0;
    for (handleId = 0; handleId < MAX_BUFFER_HANDLE; handleId++)
    {
        gRbInfo[handleId].pBufferBegin = NULL;
        gRbInfo[handleId].pWriter = NULL;
        gRbInfo[handleId].pReader = NULL;
        gRbInfo[handleId].size = 0;
        gRbInfo[handleId].dataLen[0] = 0;
        gRbInfo[handleId].readIndex = 0;
        gRbInfo[handleId].writeIndex = 0;
        gRbInfo[handleId].bufferHandle = INVALID_BUFFER_HANDLE;
        gRbInfo[handleId].fragmentedDataF = c_FALSE;
        gRbInfo[handleId].fragmentedDataPtr = NULL;
        gRbInfo[handleId].readCommittedF = c_TRUE;
    }
}

//----------------------------------------------------------------------------
/**
 * @brief Deinitialize buffer module
 */
void Rb_DeinitModule(void)
{
    cU8_t handleId = 0;
    for (handleId = 0; handleId < MAX_BUFFER_HANDLE; handleId++)
    {
        if (gRbInfo[handleId].pBufferBegin != NULL)
        {
            FREE_MEMORY(gRbInfo[handleId].pBufferBegin);
        }

        if (gRbInfo[handleId].fragmentedDataPtr != NULL)
        {
            FREE_MEMORY(gRbInfo[handleId].fragmentedDataPtr);
        }
    }
}

//----------------------------------------------------------------------------
/**
 * @brief Get a buffer instance with the specified size.
 * @param bufferSizeInBytes Size of the buffer in bytes.
 * @param bufferHandle Pointer to store the handle of the created buffer.
 * @return cBool Returns c_TRUE if the buffer instance is created successfully, otherwise c_FALSE
 */
cBool Rb_CreateBuffer(cU64_t bufferSizeInBytes, cI32_t *bufferHandle)
{
    cU8_t handleId = 0;

    if (bufferSizeInBytes > MAX_ALLOWED_BUFFER_SIZE_IN_BYTES)
    {
        EPRINT("buffer size exceeds maximum allowed size of %llu bytes", MAX_ALLOWED_BUFFER_SIZE_IN_BYTES);
        return c_FALSE;
    }

    for (handleId = 0; handleId < MAX_BUFFER_HANDLE; handleId++)
    {
        if (gRbInfo[handleId].bufferHandle == INVALID_BUFFER_HANDLE)
        {
            gRbInfo[handleId].pBufferBegin = (cU8_t *)malloc(bufferSizeInBytes);
            if (gRbInfo[handleId].pBufferBegin == NULL)
            {
                EPRINT("failed to allocate memory for buffer");
                return c_FALSE;
            }

            gRbInfo[handleId].pWriter = gRbInfo[handleId].pBufferBegin;
            gRbInfo[handleId].pReader = gRbInfo[handleId].pBufferBegin;
            gRbInfo[handleId].dataLen[0] = 0;
            gRbInfo[handleId].size = bufferSizeInBytes;
            gRbInfo[handleId].readIndex = 0;
            gRbInfo[handleId].writeIndex = 0;
            gRbInfo[handleId].bufferHandle = handleId;
            gRbInfo[handleId].fragmentedDataF = c_FALSE;
            gRbInfo[handleId].fragmentedDataPtr = NULL;
            gRbInfo[handleId].readCommittedF = c_TRUE;

            *bufferHandle = handleId;
            return c_TRUE;
        }
    }

    EPRINT("maximum buffer handles reached: [maxHandles=%d]", MAX_BUFFER_HANDLE);
    return c_FALSE;  // No available buffer handle
}

//----------------------------------------------------------------------------
/**
 * @brief Get the count of unread indices in the buffer.
 * @param bufferHandle Handle of the buffer.
 * @return cU64_t Returns the count of unread indices in the buffer.
 */
cU64_t Rb_GetUnreadIndexCount(cI32_t bufferHandle)
{
    Rb_Info_t *rbInfo = &gRbInfo[bufferHandle];

    if (rbInfo->readIndex > rbInfo->writeIndex)
    {
        return (MAX_DATA_INDEX - (rbInfo->readIndex - rbInfo->writeIndex));
    }
    else
    {
        return (rbInfo->writeIndex - rbInfo->readIndex);
    }
}

//----------------------------------------------------------------------------
/**
 * @brief Write data to the buffer.
 * @param bufferHandle Handle of the buffer to write to.
 * @param data Pointer to the data to write.
 * @param dataBytes Size of the data in bytes.
 * @return cBool Returns c_TRUE if the data is written successfully, otherwise c_FALSE.
 */
cBool Rb_WriteToBuffer(cI32_t bufferHandle, const cU8_t *data, cU64_t dataBytes)
{
    if (IS_VALID_BUFFER_HANDLE(bufferHandle) == c_FALSE)
    {
        EPRINT("invalid buffer handle: [bufferHandle=%d]", bufferHandle);
        return c_FALSE;
    }

    if ((dataBytes == 0) || (data == NULL))
    {
        EPRINT("invalid data or data size: [dataBytes=%lu]", dataBytes);
        return c_FALSE;
    }

    Rb_Info_t   *rbInfo = &gRbInfo[bufferHandle];
    cU64_t       totalFreeSpace = getFreeSpace(bufferHandle);
    cU64_t       contiguousFreeSpace = getContiguousFreeSpace(bufferHandle);
    const cU8_t *tDataPtr = data;

    if (isFreeDataIndexAvailable(bufferHandle) == c_FALSE)
    {
        EPRINT("max data index reached");
        return c_FALSE;
    }

    if (totalFreeSpace < dataBytes)
    {
        EPRINT("not enough free space in buffer: [dataBytes=%lu], [freeSpace=%lu]", dataBytes, totalFreeSpace);
        return c_FALSE;
    }

    if (contiguousFreeSpace < dataBytes)
    {
        memcpy(rbInfo->pWriter, tDataPtr, contiguousFreeSpace);
        rbInfo->dataLen[rbInfo->writeIndex] = contiguousFreeSpace;
        rbInfo->writeIndex++;

        // Update pointer and size to write remaining data
        tDataPtr += contiguousFreeSpace;
        dataBytes -= contiguousFreeSpace;

        // Wrap around
        rbInfo->pWriter = rbInfo->pBufferBegin;
        rbInfo->fragmentedDataF = c_TRUE;
    }

    memcpy(rbInfo->pWriter, tDataPtr, dataBytes);
    rbInfo->dataLen[rbInfo->writeIndex] = dataBytes;
    rbInfo->writeIndex++;
    rbInfo->pWriter += dataBytes;

    return c_TRUE;
}

//----------------------------------------------------------------------------
/**
 * @brief Read data from the buffer.
 * @param bufferHandle Handle of the buffer to read from.
 * @param data Pointer to store the read data.
 * @param dataBytes Pointer to store the size of the read data in bytes.
 * @return cBool Returns c_TRUE if the data is read successfully, otherwise c_FALSE.
 */
cBool Rb_PeekRead(cI32_t bufferHandle, cU8_t **readPtr, cU64_t *dataBytes)
{
    if (IS_VALID_BUFFER_HANDLE(bufferHandle) == c_FALSE)
    {
        EPRINT("invalid buffer handle: [bufferHandle=%d]", bufferHandle);
        return c_FALSE;
    }

    if ((dataBytes == NULL) || (*dataBytes == 0) || (readPtr == NULL))
    {
        EPRINT("invalid data or data size: [dataBytes=%lu]", *dataBytes);
        return c_FALSE;
    }

    Rb_Info_t *rbInfo = &gRbInfo[bufferHandle];

    if (rbInfo->readCommittedF == c_FALSE)
    {
        EPRINT("previous read not committed");
        return c_FALSE;
    }

    rbInfo->readCommittedF = c_FALSE;

    if (rbInfo->dataLen[rbInfo->readIndex] == 0)
    {
        DPRINT("no data available to read");
        *dataBytes = 0;
        return c_TRUE;
    }

    // Check if reading fragmented data
    if (((rbInfo->pReader + rbInfo->dataLen[rbInfo->readIndex]) == (rbInfo->pBufferBegin + rbInfo->size)) && (rbInfo->fragmentedDataF == c_TRUE))
    {
       return handleFragmentedRead(rbInfo, readPtr, dataBytes);
    }

    *readPtr = rbInfo->pReader;
    *dataBytes = rbInfo->dataLen[rbInfo->readIndex];
    return c_TRUE;
}

//----------------------------------------------------------------------------
/**
 * @brief Commit the read operation from the buffer.
 * @param bufferHandle Handle of the buffer.
 * @param dataBytes Size of the data read in bytes.
 * @return cBool Returns c_TRUE if the read is committed successfully, otherwise c_FALSE
 *
 * @todo Add partial read support within a data chunk
 */
cBool Rb_CommitRead(cI32_t bufferHandle, cU64_t dataBytes)
{
    if (IS_VALID_BUFFER_HANDLE(bufferHandle) == c_FALSE)
    {
        EPRINT("invalid buffer handle: [bufferHandle=%d]", bufferHandle);
        return c_FALSE;
    }

    Rb_Info_t *rbInfo = &gRbInfo[bufferHandle];

    if (rbInfo->readCommittedF == c_TRUE)
    {
        EPRINT("no peek read has been performed");
        return c_FALSE;
    }

    rbInfo->readCommittedF = c_TRUE;

    if (dataBytes == 0)
    {
        EPRINT("invalid data size: [dataBytes=%lu]", dataBytes);
        return c_FALSE;
    }

    /* Note: If the data was fragmented during write, we allocated memory to hold the fragmented data
     *       during peek read, so we will just free that memory during commit read and return as all
     *       pointers & indices are already updated in peek read.
     */
    if (rbInfo->fragmentedDataPtr != NULL)
    {
        FREE_MEMORY(rbInfo->fragmentedDataPtr);

        rbInfo->fragmentedDataF = c_FALSE;

        return c_TRUE;
    }

    if (dataBytes != rbInfo->dataLen[rbInfo->readIndex])
    {
        EPRINT("data size to commit does not match the peeked data size: [dataBytes=%lu], [peekedDataSize=%lu]", dataBytes,
               rbInfo->dataLen[rbInfo->readIndex]);
        return c_FALSE;
    }

    rbInfo->dataLen[rbInfo->readIndex] = 0;
    rbInfo->pReader += dataBytes;

    if (rbInfo->readIndex == (MAX_DATA_INDEX - 1))
    {
        rbInfo->readIndex = 0;
    }
    else
    {
        rbInfo->readIndex++;
    }

    return c_TRUE;
}

//----------------------------------------------------------------------------
/**
 * @brief Handle reading fragmented data from the buffer.
 * @param rbInfo Pointer to the ring buffer information.
 * @param readPtr Pointer to store the read pointer.
 * @param dataBytes Pointer to store the size of the read data in bytes.
 * @return cBool Returns c_TRUE if the fragmented data is handled successfully, otherwise c_FALSE.
 */
static cBool handleFragmentedRead(Rb_Info_t *rbInfo, cU8_t **readPtr, cU64_t *dataBytes)
{
    cU64_t part1Bytes, part2Bytes;

    if (rbInfo->readIndex >= MAX_DATA_INDEX)
    {
        // Wrap around
        rbInfo->readIndex = 0;

        part1Bytes = rbInfo->dataLen[rbInfo->readIndex];
        rbInfo->dataLen[rbInfo->readIndex] = 0;
        rbInfo->readIndex++;

        part2Bytes = rbInfo->dataLen[rbInfo->readIndex];
        rbInfo->dataLen[rbInfo->readIndex] = 0;
        rbInfo->readIndex++;
    }
    else if ((rbInfo->readIndex + 1) >= MAX_DATA_INDEX)
    {
        part1Bytes = rbInfo->dataLen[rbInfo->readIndex];
        rbInfo->dataLen[rbInfo->readIndex] = 0;

        // Wrap around
        rbInfo->readIndex = 0;

        part2Bytes = rbInfo->dataLen[rbInfo->readIndex];
        rbInfo->dataLen[rbInfo->readIndex] = 0;
        rbInfo->readIndex++;
    }
    else
    {
        // Normal case
        part1Bytes = rbInfo->dataLen[rbInfo->readIndex];
        rbInfo->dataLen[rbInfo->readIndex] = 0;
        rbInfo->readIndex++;

        part2Bytes = rbInfo->dataLen[rbInfo->readIndex];
        rbInfo->dataLen[rbInfo->readIndex] = 0;
        rbInfo->readIndex++;
    }

    // Allocate memory to hold the fragmented data
    rbInfo->fragmentedDataPtr = (cU8_t *)malloc(part1Bytes + part2Bytes);

    if (rbInfo->fragmentedDataPtr == NULL)
    {
        EPRINT("failed to allocate memory for reading fragmented data");
        return c_FALSE;
    }

    // Copy fragmented data into the allocated memory
    memcpy(rbInfo->fragmentedDataPtr, rbInfo->pReader, part1Bytes);
    rbInfo->pReader = rbInfo->pBufferBegin;
    memcpy((rbInfo->fragmentedDataPtr + part1Bytes), rbInfo->pReader, part2Bytes);
    rbInfo->pReader += part2Bytes;

    *readPtr = rbInfo->fragmentedDataPtr;
    *dataBytes = (part1Bytes + part2Bytes);

    return c_TRUE;
}

//------------------------------------------------------------------------------
/**
 * @brief Check if there is a free data index available in the buffer.
 * @param bufferHandle Handle of the buffer.
 * @return cBool Returns c_TRUE if a free data index is available, otherwise c_FALSE
 */
static cBool isFreeDataIndexAvailable(cI32_t bufferHandle)
{
    Rb_Info_t *rbInfo = &gRbInfo[bufferHandle];

    return ((getOccupiedSpace(bufferHandle) > 0) && (rbInfo->readIndex == rbInfo->writeIndex));
}

//----------------------------------------------------------------------------
/**
 * @brief Get contiguous free size in the buffer.
 * @param bufferHandle Handle of the buffer.
 * @return cU64_t Returns the free size in bytes.
 */
static cU64_t getContiguousFreeSpace(cI32_t bufferHandle)
{
    Rb_Info_t *rbInfo = &gRbInfo[bufferHandle];

    if (rbInfo->pWriter < rbInfo->pReader)
    {
        return (rbInfo->pReader - rbInfo->pWriter);
    }

    return ((rbInfo->pBufferBegin + rbInfo->size) - rbInfo->pWriter);
}

//----------------------------------------------------------------------------
/**
 * @brief Get free size in the buffer.
 * @param bufferHandle Handle of the buffer.
 * @return cU64_t Returns the free size in bytes.
 */
static cU64_t getFreeSpace(cI32_t bufferHandle)
{
    Rb_Info_t *rbInfo = &gRbInfo[bufferHandle];

    if (rbInfo->pWriter < rbInfo->pReader)
    {
        return (rbInfo->pReader - rbInfo->pWriter);
    }

    return (rbInfo->size - (rbInfo->pWriter - rbInfo->pReader));
}

//----------------------------------------------------------------------------
/**
 * @brief Get occupied size in the buffer.
 * @param bufferHandle Handle of the buffer.
 * @return cU64_t Returns the occupied size in bytes.
 */
static cU64_t getOccupiedSpace(cI32_t bufferHandle)
{
    Rb_Info_t *rbInfo = &gRbInfo[bufferHandle];

    return (rbInfo->size - getFreeSpace(bufferHandle));
}


/*****************************************************************************
 * @END OF FILE
 *****************************************************************************/
