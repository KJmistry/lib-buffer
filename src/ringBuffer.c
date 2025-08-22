/*****************************************************************************
 * @file    ringBuffer.c
 * @author  Kshitij Mistry
 * @brief
 *
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
static cU64_t getContiguousFreeSpace(cI32_t bufferHandle);

static cU64_t getFreeSpace(cI32_t bufferHandle);

__attribute__((unused)) static cU64_t getOccupiedSpace(cI32_t bufferHandle);

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

            *bufferHandle = handleId;
            return c_TRUE;
        }
    }

    EPRINT("maximum buffer handles reached: [maxHandles=%d]", MAX_BUFFER_HANDLE);
    return c_FALSE;  // No available buffer handle
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

    if ((rbInfo->readIndex > rbInfo->writeIndex) && (rbInfo->writeIndex + 1) == rbInfo->readIndex)
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
    rbInfo->pWriter += dataBytes;
    rbInfo->writeIndex++;

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
cBool Rb_ReadFromBuffer(cI32_t bufferHandle, cU8_t *data, cU64_t *dataBytes)
{
    if (IS_VALID_BUFFER_HANDLE(bufferHandle) == c_FALSE)
    {
        EPRINT("invalid buffer handle: [bufferHandle=%d]", bufferHandle);
        return c_FALSE;
    }

    if ((dataBytes == NULL) || (*dataBytes == 0) || (data == NULL))
    {
        EPRINT("invalid data or data size: [dataBytes=%lu]", *dataBytes);
        return c_FALSE;
    }

    Rb_Info_t *rbInfo = &gRbInfo[bufferHandle];

    if (rbInfo->dataLen[rbInfo->readIndex] == 0)
    {
        DPRINT("no data available to read");
        *dataBytes = 0;
        return c_TRUE;
    }

    // Check if reading fragmented data
    if (((rbInfo->pReader + rbInfo->dataLen[rbInfo->readIndex]) == (rbInfo->pBufferBegin + rbInfo->size)) && (rbInfo->fragmentedDataF == c_TRUE))
    {
        memcpy(data, rbInfo->pReader, rbInfo->dataLen[rbInfo->readIndex]);
        *dataBytes = rbInfo->dataLen[rbInfo->readIndex];
        rbInfo->dataLen[rbInfo->readIndex] = 0;
        rbInfo->readIndex++;

        if (rbInfo->dataLen[rbInfo->readIndex] == 0)
        {
            EPRINT("fragmented data not found");
            rbInfo->fragmentedDataF = c_FALSE;
            *dataBytes = 0;
            return c_FALSE;
        }

        // Wrap around
        rbInfo->pReader = rbInfo->pBufferBegin;

        memcpy((data + *dataBytes), rbInfo->pReader, rbInfo->dataLen[rbInfo->readIndex]);
        *dataBytes += rbInfo->dataLen[rbInfo->readIndex];
        rbInfo->dataLen[rbInfo->readIndex] = 0;
        rbInfo->readIndex++;
        rbInfo->pReader += rbInfo->dataLen[rbInfo->readIndex];

        // Reset the fragmentation flag
        rbInfo->fragmentedDataF = c_FALSE;
    }
    else
    {
        memcpy(data, rbInfo->pReader, rbInfo->dataLen[rbInfo->readIndex]);
        *dataBytes = rbInfo->dataLen[rbInfo->readIndex];
        rbInfo->dataLen[rbInfo->readIndex] = 0;
        rbInfo->pReader += *dataBytes;
    }

    return c_TRUE;
}

//----------------------------------------------------------------------------
/**
 * @brief Get a pointer to the write position in the buffer and the available size for writing.
 * @param bufferHandle Handle of the buffer.
 * @param writePtr Pointer to store the write pointer.
 * @param contiguousFreeSpace Pointer to store contiguous available size for writing.
 * @param totalFreeSpace Pointer to store the total available size for writing.
 * @return cBool Returns c_TRUE if the write pointer is obtained successfully, otherwise c_FALSE
 */
cBool Rb_PeekWrite(cI32_t bufferHandle, cU8_t **writePtr, cU64_t *contiguousFreeSpace, cU64_t *totalFreeSpace)
{
    if (IS_VALID_BUFFER_HANDLE(bufferHandle) == c_FALSE)
    {
        EPRINT("invalid buffer handle: [bufferHandle=%d]", bufferHandle);
        return c_FALSE;
    }

    if ((writePtr == NULL) || (contiguousFreeSpace == NULL) || (totalFreeSpace == NULL))
    {
        EPRINT("invalid write pointer or available size pointer");
        return c_FALSE;
    }

    Rb_Info_t *rbInfo = &gRbInfo[bufferHandle];

    if ((rbInfo->readIndex > rbInfo->writeIndex) && (rbInfo->writeIndex + 1) == rbInfo->readIndex)
    {
        EPRINT("max data index reached");
        *contiguousFreeSpace = 0;
        *totalFreeSpace = 0;

        return c_TRUE;
    }

    *writePtr = rbInfo->pWriter;
    *contiguousFreeSpace = getContiguousFreeSpace(bufferHandle);
    *totalFreeSpace = getFreeSpace(bufferHandle);

    return c_TRUE;
}

//----------------------------------------------------------------------------
/**
 * @brief Commit the write operation to the buffer.
 * @param bufferHandle Handle of the buffer.
 * @param dataBytes Size of the data written in bytes.
 * @param fragmentedDataF Pointer to store the fragmentation flag.
 * @return cBool Returns c_TRUE if the write is committed successfully, otherwise c_FALSE
 */
cBool Rb_CommitWrite(cI32_t bufferHandle, cU64_t dataBytes, cBool *fragmentedDataF)
{
    if (IS_VALID_BUFFER_HANDLE(bufferHandle) == c_FALSE)
    {
        EPRINT("invalid buffer handle: [bufferHandle=%d]", bufferHandle);
        return c_FALSE;
    }

    Rb_Info_t *rbInfo = &gRbInfo[bufferHandle];

    if (dataBytes == 0)
    {
        EPRINT("invalid data size: [dataBytes=%lu]", dataBytes);
        return c_FALSE;
    }

    if (dataBytes > getContiguousFreeSpace(bufferHandle))
    {
        EPRINT("written data exceeds contiguous free space: [dataBytes=%lu], [contiguousFreeSpace=%lu]", dataBytes,
               getContiguousFreeSpace(bufferHandle));
        return c_FALSE;
    }

    rbInfo->pWriter += dataBytes;
    rbInfo->dataLen[rbInfo->writeIndex] = dataBytes;

    if (rbInfo->writeIndex == (MAX_DATA_INDEX - 1))
    {
        rbInfo->writeIndex = 0;  // Wrap around
    }
    else
    {
        rbInfo->writeIndex++;
    }

    if (fragmentedDataF != NULL)
    {
        rbInfo->fragmentedDataF = *fragmentedDataF;
    }

    return c_TRUE;
}

//----------------------------------------------------------------------------
/**
 * @brief Get a pointer to the read position in the buffer and the available size for reading.
 * @param bufferHandle Handle of the buffer.
 * @param readPtr Pointer to store the read pointer.
 * @param dataBytes Pointer to store the available size for reading.
 * @param fragmentedDataF Pointer to store the fragmentation flag.
 * @return cBool Returns c_TRUE if the read pointer is obtained successfully, otherwise c_FALSE
 */
cBool Rb_PeekRead(cI32_t bufferHandle, cU8_t **readPtr, cU64_t *dataBytes, cBool *fragmentedDataF)
{
    if (IS_VALID_BUFFER_HANDLE(bufferHandle) == c_FALSE)
    {
        EPRINT("invalid buffer handle: [bufferHandle=%d]", bufferHandle);
        return c_FALSE;
    }

    if ((readPtr == NULL) || (dataBytes == NULL))
    {
        EPRINT("invalid read pointer or available size pointer");
        return c_FALSE;
    }

    Rb_Info_t *rbInfo = &gRbInfo[bufferHandle];

    *readPtr = rbInfo->pReader;
    *dataBytes = rbInfo->dataLen[rbInfo->readIndex];

    // Set fragmentation flag only if this is the last chunk of data in the buffer
    if ((rbInfo->pReader + rbInfo->dataLen[rbInfo->readIndex]) == (rbInfo->pBufferBegin + rbInfo->size))
    {
        *fragmentedDataF = rbInfo->fragmentedDataF;
    }
    else
    {
        *fragmentedDataF = c_FALSE;
    }

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

    if (dataBytes == 0)
    {
        EPRINT("invalid data size: [dataBytes=%lu]", dataBytes);
        return c_FALSE;
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
        rbInfo->readIndex = 0;  // Wrap around
    }
    else
    {
        rbInfo->readIndex++;
    }

    return c_TRUE;
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
