/*****************************************************************************
 * @file    buffer.h
 * @author  Kshitij Mistry
 * @brief
 *
 *
 *****************************************************************************/
#pragma once

/*****************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "common_stddef.h"

#define MAX_DATA_INDEX (1000LL) /**< Maximum number of data indices in the ring buffer */
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
 * FUNCTION DECLARATIONS
 *****************************************************************************/
void Rb_InitModule(void);

void Rb_DeinitModule(void);

cBool Rb_CreateBuffer(cU64_t bufferSizeInBytes, cI32_t *bufferHandle);

cBool Rb_DestroyBuffer(cI32_t *bufferHandle);

cU64_t Rb_GetUnreadIndexCount(cI32_t bufferHandle);

cBool Rb_GetFreeSpace(cI32_t bufferHandle, cU64_t *freeSpace);

/** Zero copy read/write APIs */
cBool Rb_WriteToBuffer(cI32_t bufferHandle, const cU8_t *data, cU64_t dataSize);

cBool Rb_PeekRead(cI32_t bufferHandle, cU8_t **readPtr, cU64_t *dataBytes);

cBool Rb_CommitRead(cI32_t bufferHandle, cU64_t dataBytes);

/*****************************************************************************
 * @END OF FILE
 *****************************************************************************/
