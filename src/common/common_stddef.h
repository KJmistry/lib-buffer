/*****************************************************************************
 * @file    common_stddef.h
 * @author  Kshitij Mistry
 * @brief   Custom data types and return value status codes.
 *
 * This header file defines custom data types and boolean status codes used throughout the project.
 * It is designed to be compatible with C++14 and includes necessary type definitions for various
 * integer sizes, floating-point numbers, and a boolean type.
 *
 *****************************************************************************/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <inttypes.h>
#include <stddef.h>

/*****************************************************************************
 * TYPEDEFS
 *****************************************************************************/
/**
 * @brief Boolean type, represents true or false.
 */
typedef unsigned char cBool;

/**
 * @brief Character type.
 */
typedef char cChar;

/**
 * @brief Unsigned character type
 */
typedef unsigned char cUchar;

/**
 * @brief 8-bit signed integer
 */
typedef int8_t cI8_t;

/**
 * @brief 16-bit signed integer
 */
typedef int16_t cI16_t;

/**
 * @brief 32-bit signed integer
 */
typedef int32_t cI32_t;

/**
 * @brief 64-bit signed integer
 */
typedef int64_t cI64_t;

/**
 * @brief 8-bit unsigned integer
 */
typedef uint8_t cU8_t;

/**
 * @brief 16-bit unsigned integer
 */
typedef uint16_t cU16_t;

/**
 * @brief 32-bit unsigned integer
 */
typedef uint32_t cU32_t;

/**
 * @brief 64-bit unsigned integer
 */
typedef uint64_t cU64_t;

/**
 * @brief Single-precision floating point number
 */
typedef float cFloat_t;

/**
 * @brief Long integer
 */
typedef long cLong_t;

/**
 * @brief Double-precision floating point number
 */
typedef double cDouble_t;

/**
 * @brief Long long integer
 */
typedef long long cLL_t;

/**
 * @brief Unsigned integer type used to represent sizes
 */
typedef size_t cSize_t;

/*****************************************************************************
 * ENUMS
 *****************************************************************************/
/**
 * @brief Status codes used in network manager operations.
 */
typedef enum
{
    cStatus_SUCCESS,                  /**< Operation completed successfully */
    cStatus_IN_PROGRESS,              /**< Operation is in progress */
    cStatus_UNKNOWN_ERR,              /**< Error that is not known or categorized */
    cStatus_UNEXPECTED_ERR,           /**< Unexpected error occurred */
    cStatus_INVALID_ARG,              /**< Invalid argument provided */
    cStatus_NO_RESOURCE,              /**< Insufficient resources available */
    cStatus_RESOURCE_BUSY,            /**< Resource is busy */
    cStatus_CONNECTION_ERR,           /**< Socket connection error */
    cStatus_OPERATION_FAIL,           /**< Operation failed */
    cStatus_OPERATION_TIMEOUT,        /**< Operation timed out */
    cStatus_FILE_IO_ERR,              /**< File I/O error */

} cStatus_e;

 /**
 * @brief Boolean status codes.
 */
enum
{
    c_FALSE = 0,   /**< Represents boolean false */
    c_TRUE = 1     /**< Represents boolean true */
};

#ifdef __cplusplus
}
#endif
/*****************************************************************************
 * @END OF FILE
 *****************************************************************************/
