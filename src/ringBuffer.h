/*****************************************************************************
 * @file    buffer.h
 * @author  Kshitij Mistry
 * @brief   Header file for ring buffer implementation
 *****************************************************************************/
#pragma once

/*****************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "common_stddef.h"

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
