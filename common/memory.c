#include "includes.h"

uint32_t
coapi_allocate_memory(
    size_t size,
    void** ppMemory
    )
{
    uint32_t dwError = 0;
    void* pMemory = NULL;

    if (!ppMemory || !size)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pMemory = calloc(1, size);
    if (!pMemory)
    {
        dwError = ENOMEM;
        BAIL_ON_ERROR(dwError);
    }

    *ppMemory = pMemory;

cleanup:
    return dwError;

error:
    if (ppMemory)
    {
        *ppMemory = NULL;
    }
    coapi_free_memory(pMemory);
    goto cleanup;
}

void
coapi_free_memory(
    void* pMemory
    )
{
    if(pMemory)
    {
        free(pMemory);
    }
}
