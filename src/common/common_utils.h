/*****************************************************************************
 * @file    common_utils.h
 * @author  Kshitij Mistry
 * @brief   Common utility functions for the scheduler module.
 *
 * This header file declares utility functions for handling time-related operations,
 * such as sleeping for a specified duration, getting monotonic time, and
 * retrieving the current time since the epoch. These functions are used by
 * the scheduler module to manage task execution intervals and timing.
 *
 *****************************************************************************/
#pragma once

/*****************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <time.h>
#include "common_stddef.h"

/*****************************************************************************
 * FUNCTION PROTOTYPES
 *****************************************************************************/

void Utils_SleepNanoSec(cU64_t ns);

cU64_t Utils_GetMonotonicTimeInNanoSec(void);

cU64_t Utils_GetMonotonicTimeInSec(void);

void Utils_GetTimeSinceEpoch(struct tm *pTime);

/*****************************************************************************
 * @END OF FILE
 *****************************************************************************/
