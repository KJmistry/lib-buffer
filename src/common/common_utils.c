/*****************************************************************************
 * @file    common_utils.c
 * @author  Kshitij Mistry
 * @brief   Common utility functions for the scheduler module.
 *
 * This file contains utility functions for handling time-related operations,
 * such as sleeping for a specified duration, getting monotonic time, and
 * retrieving the current time since the epoch. These functions are used by
 * the scheduler module to manage task execution intervals and timing.
 *
 *****************************************************************************/

/*****************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "common_utils.h"
#include <errno.h>

/*****************************************************************************
 * MACROS
 *****************************************************************************/
/** Nano seconds in 1 sec */
#define NANO_SECONDS_PER_SECOND (1000000000LL)

/*****************************************************************************
 * FUNCTION DEFINATIONS
 *****************************************************************************/
//----------------------------------------------------------------------------
/**
 * @brief Takes amount of time for sleep in nano second
 * @param ns count for sleep
 */
void Utils_SleepNanoSec(cU64_t ns)
{
    struct timespec req = {0, 0};

    if (ns >= NANO_SECONDS_PER_SECOND)
    {
        req.tv_sec = (ns / NANO_SECONDS_PER_SECOND);
        req.tv_nsec = (ns % NANO_SECONDS_PER_SECOND);
    }
    else
    {
        req.tv_nsec = ns;
    }

    /* sleep until next tick */
    while ((-1 == nanosleep(&req, &req)) && (EINTR == errno))
    {
        continue;
    }
}

//----------------------------------------------------------------------------
/**
 * @brief Function that gets the time elapsed since system power on in nanosec
 * @return Elapsed time in nanosec
 */
cU64_t Utils_GetMonotonicTimeInNanoSec(void)
{
    struct timespec ts;

    /* Get time in form of sec & nano sec */
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (cU64_t)((((cU64_t)ts.tv_sec) * 1000000000LL) + ts.tv_nsec);
}

//----------------------------------------------------------------------------
/**
 * @brief Function that gets the time elapsed since system power on in sec
 * @return Elapsed time in sec
 */
cU64_t Utils_GetMonotonicTimeInSec(void)
{
    struct timespec ts;

    /* Get time in form of sec & nano sec */
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (cU64_t)(ts.tv_sec);
}

//----------------------------------------------------------------------------
/**
 * @brief To get current broken time and date of system
 * @param pTime structure to which broken time will get copied
 */
void Utils_GetTimeSinceEpoch(struct tm *pTime)
{
    time_t timeSinceEpoch = 0;

    time(&timeSinceEpoch);

    localtime_r(&timeSinceEpoch, pTime);
}

/*****************************************************************************
 * @END OF FILE
 *****************************************************************************/
