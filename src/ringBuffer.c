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
cBool Rb_GetBufferInstance(cU64_t bufferSizeInBytes, cI32_t *bufferHandle)
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

            *bufferHandle = handleId;
            return c_TRUE;
        }
    }

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

    Rb_Info_t *rbInfo = &gRbInfo[bufferHandle];

    // TODO: Handle buffer overflow & wrap-around logic

    memcpy(rbInfo->pBufferBegin, data, dataBytes);
    rbInfo->dataLen[rbInfo->writeIndex] = dataBytes;
    rbInfo->pWriter = rbInfo->pWriter + dataBytes;
    rbInfo->writeIndex++;

    return c_TRUE;
}

//----------------------------------------------------------------------------
/**
 * @brief Get a pointer to the write position in the buffer and the available size for writing.
 * @param bufferHandle Handle of the buffer.
 * @param writePtr Pointer to store the write pointer.
 * @param availableSize Pointer to store the available size for writing.
 * @return cBool Returns c_TRUE if the write pointer is obtained successfully, otherwise c_FALSE
 */
cBool Rb_GetWritePtr(cI32_t bufferHandle, cU8_t **writePtr, cU64_t *availableSize)
{
    if (IS_VALID_BUFFER_HANDLE(bufferHandle) == c_FALSE)
    {
        EPRINT("invalid buffer handle: [bufferHandle=%d]", bufferHandle);
        return c_FALSE;
    }

    if ((writePtr == NULL) || (availableSize == NULL))
    {
        EPRINT("invalid write pointer or available size pointer");
        return c_FALSE;
    }

    Rb_Info_t *rbInfo = &gRbInfo[bufferHandle];

    *writePtr = rbInfo->pWriter;
    // *availableSize = getFreeSize();  // TODO
    return c_TRUE;
}

//----------------------------------------------------------------------------
/**
 * @brief Commit the write operation to the buffer.
 * @param bufferHandle Handle of the buffer.
 * @param dataBytes Size of the data written in bytes.
 * @return cBool Returns c_TRUE if the write is committed successfully, otherwise c_FALSE
 */
cBool Rb_CommitWrite(cI32_t bufferHandle, cU64_t dataBytes)
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

    if ((rbInfo->pWriter + dataBytes) > (rbInfo->pBufferBegin + rbInfo->size))
    {
        EPRINT("write exceeds buffer size: [dataBytes=%lu, bufferSize=%lu]", dataBytes, rbInfo->size);
        return c_FALSE;
    }

    rbInfo->pWriter += dataBytes;
    rbInfo->dataLen[rbInfo->writeIndex] = dataBytes;
    rbInfo->writeIndex++;  // TODO: Handle wrap-around logic
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

    // Rb_Info_t *rbInfo = &gRbInfo[bufferHandle];

    return c_FALSE;  // TODO: Implement read logic
}

//----------------------------------------------------------------------------
/**
 * @brief Get a pointer to the read position in the buffer and the available size for reading.
 * @param bufferHandle Handle of the buffer.
 * @param readPtr Pointer to store the read pointer.
 * @param dataBytes Pointer to store the available size for reading.
 * @return cBool Returns c_TRUE if the read pointer is obtained successfully, otherwise c_FALSE
 */
cBool Rb_GetReadPtr(cI32_t bufferHandle, cU8_t **readPtr, cU64_t *dataBytes)
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
    return c_TRUE;
}

//----------------------------------------------------------------------------
/**
 * @brief Commit the read operation from the buffer.
 * @param bufferHandle Handle of the buffer.
 * @param dataBytes Size of the data read in bytes.
 * @return cBool Returns c_TRUE if the read is committed successfully, otherwise c_FALSE
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

    if ((rbInfo->pReader + dataBytes) > (rbInfo->pBufferBegin + rbInfo->size))
    {
        EPRINT("read exceeds buffer size: [dataBytes=%lu, bufferSize=%lu]", dataBytes, rbInfo->size);
        return c_FALSE;
    }
    rbInfo->pReader += dataBytes;
    rbInfo->readIndex += dataBytes;
    rbInfo->dataLen[rbInfo->readIndex] = dataBytes;
    return c_TRUE;
}

/*****************************************************************************
 * @END OF FILE
 *****************************************************************************/
