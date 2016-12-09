#include "includes.h"

uint32_t
coapi_allocate_string(
    const char* pszSrc,
    char** ppszDest
    )
{
    uint32_t dwError = 0;
    char* pszDest = NULL;
    int nLength = 0;

    if (!pszSrc || !ppszDest)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    nLength = strlen(pszSrc);

    dwError = coapi_allocate_memory(nLength + 1, (void **)&pszDest);
    BAIL_ON_ERROR(dwError);

    memcpy(pszDest, pszSrc, nLength);

    *ppszDest = pszDest;

cleanup:
    return dwError;

error:
    if(ppszDest)
    {
        *ppszDest = NULL;
    }
    goto cleanup;
}


uint32_t
coapi_allocate_string_printf(
    char** ppszDst,
    const char* pszFmt,
    ...
    )
{
    uint32_t dwError = 0;
    size_t nSize = 0;
    char* pszDst = NULL;
    char chDstTest = '\0';
    va_list argList;

    if(!ppszDst || !pszFmt)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    //Find size
    va_start(argList, pszFmt);
    nSize = vsnprintf(&chDstTest, 1, pszFmt, argList);
    va_end(argList);

    if(nSize <= 0)
    {
        dwError = errno;
        BAIL_ON_ERROR(dwError);
    }

    nSize = nSize + 1;
    dwError = coapi_allocate_memory(nSize, (void**)&pszDst);
    BAIL_ON_ERROR(dwError);

    va_start(argList, pszFmt);
    nSize = vsnprintf(pszDst, nSize, pszFmt, argList);
    va_end(argList);

    if(nSize <= 0)
    {
        dwError = errno;
        BAIL_ON_ERROR(dwError);
    }
    *ppszDst = pszDst;
cleanup:
    return dwError;

error:
    if(ppszDst)
    {
        *ppszDst = NULL;
    }
    SAFE_FREE_MEMORY(pszDst);
    goto cleanup;
}

void
coapi_free_string_array_with_count(
    char **ppszArray,
    int nCount
    )
{
    if(ppszArray)
    {
        while(nCount)
        {
            coapi_free_memory(ppszArray[--nCount]);
        }
        coapi_free_memory(ppszArray);
    }
}
