/*****************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "common_def.h"

/*****************************************************************************
 * MACROS
 *****************************************************************************/

/*****************************************************************************
 * FUNCTION DEFINATIONS
 *****************************************************************************/
/**
 * @brief EnumToStr_NMSts_e
 * @param nmSts
 * @return
 */
const cChar* EnumToStr_cStatus_e(cStatus_e cStatus)
{
    #define SKIP_PREFIX_SIZE (0)

    switch (cStatus)
    {
        CASE_TO_STR(cStatus_SUCCESS, SKIP_PREFIX_SIZE);
        CASE_TO_STR(cStatus_IN_PROGRESS, SKIP_PREFIX_SIZE);
        CASE_TO_STR(cStatus_UNKNOWN_ERR, SKIP_PREFIX_SIZE);
        CASE_TO_STR(cStatus_UNEXPECTED_ERR, SKIP_PREFIX_SIZE);
        CASE_TO_STR(cStatus_INVALID_ARG, SKIP_PREFIX_SIZE);
        CASE_TO_STR(cStatus_NO_RESOURCE, SKIP_PREFIX_SIZE);
        CASE_TO_STR(cStatus_RESOURCE_BUSY, SKIP_PREFIX_SIZE);
        CASE_TO_STR(cStatus_CONNECTION_ERR, SKIP_PREFIX_SIZE);
        CASE_TO_STR(cStatus_OPERATION_FAIL, SKIP_PREFIX_SIZE);
        CASE_TO_STR(cStatus_OPERATION_TIMEOUT, SKIP_PREFIX_SIZE);
        CASE_TO_STR(cStatus_FILE_IO_ERR, SKIP_PREFIX_SIZE);
        default:
        break;
    }

    #undef SKIP_PREFIX_SIZE

    return ("cStatus_UNKNOWN");
}