/*****************************************************************************
 * @file    ringBuffer.c
 * @author  Kshitij Mistry
 * @brief   Implementation file for ring buffer APIs
 * @todo    - Add support for self allocated buffer memory, user will provide the memory pointer
 *          and size while creating the buffer instance.
 *          - Add support for multiple readers/writers with proper synchronization.
 *          - Add support for partial read within a data chunk.
 *          - Make MAX_BUFFER_HANDLE and MAX_ALLOWED_BUFFER_SIZE_IN_BYTES configurable.
 *          - Add apis to copy data from ring buffer to user provided buffer.
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
#define _BYTES_PER_MEGA_BYTE             (1024LL*1024LL)

/** Maximum allowed buffer size in bytes */
#define MAX_ALLOWED_BUFFER_SIZE_IN_BYTES (10 * _BYTES_PER_MEGA_BYTE)  // 10 Mega Bytes

/** Invalid buffer handle */
#define INVALID_BUFFER_HANDLE            (-1)

/** Maximum number of buffer handles supported */
#define MAX_BUFFER_HANDLE                (10)

/** Check if buffer handle is valid */
#define IS_VALID_BUFFER_HANDLE(handle) \
    (((handle) >= 0) && ((handle) < MAX_BUFFER_HANDLE) && (gRbInfo[(handle)].bufferHandle != INVALID_BUFFER_HANDLE))

/** Check if reading fragmented data */
#define IS_DATA_FRAGMENTED(rbInfo) \
        (((rbInfo->pReader + rbInfo->dataLen[rbInfo->readIndex]) == (rbInfo->pBufferBegin + rbInfo->size)) && ((rbInfo)->fragmentedDataF == c_TRUE))

/** Macro to check if buffer is empty (all data has been read) */
#define IS_BUFFER_EMPTY(bufferHandle) (getFreeSpace(bufferHandle) == gRbInfo[(bufferHandle)].size)

/** Maximum number of data indices in the ring buffer */
#define MAX_DATA_INDEX (1000LL)

/*****************************************************************************
 * STRUCTURES
 *****************************************************************************/
typedef struct
{
    cU8_t *pBufferBegin;            /**< Pointer to the buffer memory */
    cU8_t *pWriter;                 /**< Pointer to the writer position in the buffer */
    cU8_t *pReader;                 /**< Pointer to the reader position in the buffer */
    cU64_t size;                    /**< Size of the buffer in bytes */
    cU64_t readIndex;               /**< Index for reading from the buffer */
    cU64_t writeIndex;              /**< Index for writing to the buffer */
    cU64_t dataLen[MAX_DATA_INDEX]; /**< Length of data at each index */
    cI32_t bufferHandle;            /**< Handle for the buffer */
    cBool  fragmentedDataF;         /**< Flag to indicate if the data is fragmented */
    cU8_t *fragmentedDataPtr;       /**< Pointer to hold fragmented data */
    cBool  readCommittedF;          /**< Flag to indicate if the read has been committed */

} Rb_Info_t;

/*****************************************************************************
 * VARIABLES
 *****************************************************************************/
Rb_Info_t gRbInfo[MAX_BUFFER_HANDLE] = {0}; /**< Ring buffer information for each user */

/*****************************************************************************
 * FUNCTION DECLARATIONS
 *****************************************************************************/
static cBool handleFragmentedPeek(Rb_Info_t *rbInfo, cU8_t **readPtr, cU64_t *dataBytes);

static void handleFragmentedCommit(Rb_Info_t *rbInfo);

static void advanceReader(Rb_Info_t *rbInfo, cU64_t dataBytes);

static void resetBuffer(Rb_Info_t *rbInfo);

static cU64_t getUnreadIndexCount(cI32_t bufferHandle);

static cU64_t getContiguousFreeSpace(cI32_t bufferHandle);

static cU64_t getFreeSpace(cI32_t bufferHandle);

static cU64_t getOccupiedSpace(cI32_t bufferHandle) __attribute__((unused));

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
 * @brief Destroy the buffer instance associated with the given handle.
 * @param bufferHandle Handle of the buffer to be destroyed.
 * @return cBool Returns c_TRUE if the buffer is destroyed successfully, otherwise c_FALSE
 */
cBool Rb_DestroyBuffer(cI32_t *bufferHandle)
{
    if (bufferHandle == NULL)
    {
        EPRINT("invalid buffer handle pointer");
        return c_FALSE;
    }

    if (IS_VALID_BUFFER_HANDLE(*bufferHandle) == c_FALSE)
    {
        EPRINT("invalid buffer handle: [bufferHandle=%d]", (*bufferHandle));
        return c_FALSE;
    }

    Rb_Info_t *rbInfo = &gRbInfo[(*bufferHandle)];

    if (rbInfo->pBufferBegin != NULL)
    {
        FREE_MEMORY(rbInfo->pBufferBegin);
        rbInfo->pBufferBegin = NULL;
    }

    if (rbInfo->fragmentedDataPtr != NULL)
    {
        FREE_MEMORY(rbInfo->fragmentedDataPtr);
        rbInfo->fragmentedDataPtr = NULL;
    }

    rbInfo->bufferHandle = INVALID_BUFFER_HANDLE;
    *bufferHandle = INVALID_BUFFER_HANDLE;

    return c_TRUE;
}

//----------------------------------------------------------------------------
/**
 * @brief Get the count of unread indices in the buffer.
 * @param bufferHandle Handle of the buffer.
 * @return cU64_t Returns the count of unread indices in the buffer.
 */
cU64_t Rb_GetUnreadIndexCount(cI32_t bufferHandle)
{
    return getUnreadIndexCount(bufferHandle);
}

//----------------------------------------------------------------------------
/**
 * @brief Get the free space in the buffer.
 * @param bufferHandle Handle of the buffer.
 * @param freeSpace Pointer to store the free space in bytes.
 * @return cBool Returns c_TRUE if the free space is retrieved successfully, otherwise c_FALSE
 */
cBool Rb_GetFreeSpace(cI32_t bufferHandle, cU64_t *freeSpace)
{
    if (IS_VALID_BUFFER_HANDLE(bufferHandle) == c_FALSE)
    {
        EPRINT("invalid buffer handle: [bufferHandle=%d]", bufferHandle);
        return c_FALSE;
    }

    if (freeSpace == NULL)
    {
        EPRINT("invalid free space pointer");
        return c_FALSE;
    }

    *freeSpace = getFreeSpace(bufferHandle);
    return c_TRUE;
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

    if (getUnreadIndexCount(bufferHandle) >= MAX_DATA_INDEX)
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

        if (rbInfo->writeIndex == MAX_DATA_INDEX)
        {
            // Wrap around
            rbInfo->writeIndex = 0;
        }

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

    if (rbInfo->writeIndex == MAX_DATA_INDEX)
    {
        // Wrap around
        rbInfo->writeIndex = 0;
    }

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

    if ((dataBytes == NULL) || (readPtr == NULL))
    {
        EPRINT("invalid data pointer");
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
        EPRINT("no data available to read");
        *dataBytes = 0;
        return c_FALSE;
    }

    // Check if reading fragmented data
    if (IS_DATA_FRAGMENTED(rbInfo))
    {
       return handleFragmentedPeek(rbInfo, readPtr, dataBytes);
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
        handleFragmentedCommit(rbInfo);
    }
    else
    {
        if (dataBytes != rbInfo->dataLen[rbInfo->readIndex])
        {
            EPRINT("data size to commit does not match the peeked data size: [dataBytes=%lu], [peekedDataSize=%lu]", dataBytes,
                   rbInfo->dataLen[rbInfo->readIndex]);
            return c_FALSE;
        }

        advanceReader(rbInfo, dataBytes);
    }

    if (IS_BUFFER_EMPTY(bufferHandle))
    {
        // All data has been read, reset indices and pointers
        resetBuffer(rbInfo);
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
static cBool handleFragmentedPeek(Rb_Info_t *rbInfo, cU8_t **readPtr, cU64_t *dataBytes)
{
    cU64_t part1Bytes, part2Bytes;

    part1Bytes = rbInfo->dataLen[rbInfo->readIndex];
    rbInfo->dataLen[rbInfo->readIndex] = 0;
    rbInfo->readIndex++;

    if (rbInfo->readIndex == MAX_DATA_INDEX)
    {
        // Wrap around
        rbInfo->readIndex = 0;
    }

    part2Bytes = rbInfo->dataLen[rbInfo->readIndex];
    rbInfo->dataLen[rbInfo->readIndex] = 0;
    rbInfo->readIndex++;

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

//----------------------------------------------------------------------------
/**
 * @brief Handle committing a read of fragmented data.
 * @param rbInfo Pointer to the ring buffer information.
 */
static void handleFragmentedCommit(Rb_Info_t *rbInfo)
{
    FREE_MEMORY(rbInfo->fragmentedDataPtr);
    rbInfo->fragmentedDataF = c_FALSE;
}

//----------------------------------------------------------------------------
/**
 * @brief Advance the reader pointer and index after committing a read.
 * @param rbInfo Pointer to the ring buffer information.
 * @param dataBytes Size of the data read in bytes.
 */
static void advanceReader(Rb_Info_t *rbInfo, cU64_t dataBytes)
{
    rbInfo->dataLen[rbInfo->readIndex] = 0;
    rbInfo->pReader += dataBytes;
    rbInfo->readIndex++;

    if (rbInfo->readIndex == MAX_DATA_INDEX)
    {
        rbInfo->readIndex = 0;
    }
}

//----------------------------------------------------------------------------
/**
 * @brief Reset the buffer pointers and indices.
 * @param rbInfo Pointer to the ring buffer information.
 */
static void resetBuffer(Rb_Info_t *rbInfo)
{
    rbInfo->pReader = rbInfo->pBufferBegin;
    rbInfo->pWriter = rbInfo->pBufferBegin;
    rbInfo->readIndex = 0;
    rbInfo->writeIndex = 0;
}

//------------------------------------------------------------------------------
/**
 * @brief Check if there is a free data index available in the buffer.
 * @param bufferHandle Handle of the buffer.
 * @return cBool Returns c_TRUE if a free data index is available, otherwise c_FALSE
 */
static cU64_t getUnreadIndexCount(cI32_t bufferHandle)
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
