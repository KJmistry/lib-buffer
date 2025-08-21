/*****************************************************************************
 * @file    common_def.h
 * @author  Kshitij Mistry
 * @brief   Common definitions and data types used across the project.
 *
 * This header file defines common data types, enumerations, and macros used throughout the project.
 *
 *****************************************************************************/
#pragma once

/*****************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <stdio.h>
#include <time.h>
#include "common_stddef.h"

/*****************************************************************************
 * MACROS
 *****************************************************************************/
// ANSI color codes
#define COLOR_RESET  "\x1b[0m"
#define COLOR_WHITE  "\x1b[37m"
#define COLOR_GREEN  "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_RED    "\x1b[31m"

// Internal macro for printing with color
#define __PRINT_WITH_COLOR(color, fmt, ...)                                                                                                          \
    do                                                                                                                                               \
    {                                                                                                                                                \
        time_t    __t = time(NULL);                                                                                                                  \
        struct tm __tm;                                                                                                                              \
        localtime_r(&__t, &__tm);                                                                                                                    \
        fprintf(stderr, "%s%02d:%02d:%02d : %s[%d] : " fmt "%s\n", color, __tm.tm_hour, __tm.tm_min, __tm.tm_sec, __func__, __LINE__, ##__VA_ARGS__, \
                COLOR_RESET);                                                                                                                        \
    } while (0)

// Public macros
#define DPRINT(fmt, ...)       __PRINT_WITH_COLOR(COLOR_WHITE, fmt, ##__VA_ARGS__)
#define IPRINT(fmt, ...)       __PRINT_WITH_COLOR(COLOR_GREEN, fmt, ##__VA_ARGS__)
#define WPRINT(fmt, ...)       __PRINT_WITH_COLOR(COLOR_YELLOW, fmt, ##__VA_ARGS__)
#define EPRINT(fmt, ...)       __PRINT_WITH_COLOR(COLOR_RED, fmt, ##__VA_ARGS__)

// Convert enums to string
#define ENUM_TO_STR_cStatus(x) EnumToStr_cStatus_e(x)

// Case to string conversion using token pasting with skipping common prefix of enum
#define CASE_TO_STR(x, skip) \
    case x:                  \
        return (#x + skip);

// Initialization of mutex
#define MUTEX_INIT(x, attr)           pthread_mutex_init(&x, attr)

// Define to acquire mutex lock
#define MUTEX_LOCK(x)                 pthread_mutex_lock(&x)

// Define to release mutex lock
#define MUTEX_UNLOCK(x)               pthread_mutex_unlock(&x)

// Validate boolean value
#define IS_VALID_BOOL(val)            ((val == c_TRUE) || (val == c_FALSE))

// Nanosecond per millisecond
#define NANO_SECONDS_PER_MILLI_SECOND (1000000LL)

// Free memory safely
#define FREE_MEMORY(pMemory)   \
    do                         \
    {                          \
        if ((pMemory) != NULL) \
        {                      \
            free(pMemory);     \
            (pMemory) = NULL;  \
        }                      \
    } while (0)

/*****************************************************************************
 * FUNCTION DECLARATIONS
 *****************************************************************************/
const cChar* EnumToStr_cStatus_e(cStatus_e cStatus);

/*****************************************************************************
 * @END OF FILE
 *****************************************************************************/
