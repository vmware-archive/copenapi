#pragma once

#define MAX_CONFIG_LINE_LENGTH 1024

#define BAIL_ON_ERROR(dwError) \
    do {                                                           \
        if (dwError)                                               \
        {                                                          \
            goto error;                                            \
        }                                                          \
    } while(0)

#define SAFE_FREE_MEMORY(pMemory) \
    do { \
        if (pMemory) { \
            coapi_free_memory(pMemory); \
        } \
    } while(0)

#define IsNullOrEmptyString(str) (!(str) || !(*str))
