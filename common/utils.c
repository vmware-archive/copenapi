#include "includes.h"

uint32_t
dup_argv(
    int argc,
    char* const* argv,
    char*** argvDup
    )
{
    uint32_t dwError = 0;
    int i = 0;
    char** dup = NULL;

    if(!argv || !argvDup)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_allocate_memory(sizeof(char*) * argc, (void**)&dup);
    BAIL_ON_ERROR(dwError);

    for(i = 0; i < argc; ++i)
    {
        dup[i] = strdup(argv[i]);
    }
    *argvDup = dup;

cleanup:
    return dwError;

error:
    if(argvDup)
    {
        *argvDup = NULL;
    }
    goto cleanup;
}

const char *
ltrim(
    const char *pszStr
    )
{
    if(!pszStr) return NULL;
    while(isspace(*pszStr)) ++pszStr;
    return pszStr;
}

const char *
rtrim(
    const char *pszStart,
    const char *pszEnd
    )
{
    if(!pszStart || !pszEnd) return NULL;
    while(pszEnd > pszStart && isspace(*pszEnd)) pszEnd--;
    return pszEnd;
}

uint32_t
count_matches(
    const char *pszString,
    const char *pszFind,
    int *pnCount
    )
{
    uint32_t dwError = 0;
    int nCount = 0;
    int nOffset = 0;
    int nFindLength = 0;
    char *pszMatch = NULL;

    if(IsNullOrEmptyString(pszString) ||
       IsNullOrEmptyString(pszFind) ||
       !pnCount)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    nFindLength = strlen(pszFind);
    while((pszMatch = strcasestr(pszString + nOffset, pszFind)))
    {
        ++nCount;
        nOffset = pszMatch - pszString + nFindLength;
    }

    *pnCount = nCount;
cleanup:
    return dwError;

error:
    if(pnCount)
    {
        *pnCount = 0;
    }
    goto cleanup;
}

uint32_t
string_replace(
    const char *pszString,
    const char *pszFind,
    const char *pszReplace,
    char **ppszResult
    )
{
    uint32_t dwError = 0;
    char *pszResult = NULL;
    char *pszBoundary = NULL;
    int nCount = 0;
    int nResultLength = 0;
    int nFindLength = 0;
    int nReplaceLength = 0;
    int nOffset = 0;

    if(IsNullOrEmptyString(pszString) ||
       IsNullOrEmptyString(pszFind) ||
       !ppszResult)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = count_matches(pszString, pszFind, &nCount);
    BAIL_ON_ERROR(dwError);

    if(nCount == 0)
    {
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }

    nFindLength = strlen(pszFind);
    if(pszReplace)
    {
        nReplaceLength = strlen(pszReplace);
    }

    nResultLength = strlen(pszString) +
                    nCount * (nReplaceLength - nFindLength);

    dwError = coapi_allocate_memory(sizeof(char) * (nResultLength + 1),
                                    (void **)&pszResult);
    BAIL_ON_ERROR(dwError);

    nOffset = 0;
    while((pszBoundary = strcasestr(pszString + nOffset, pszFind)))
    {
        int nLength = pszBoundary - (pszString + nOffset);

        strncat(pszResult, pszBoundary - nLength, nLength);
        if(pszReplace)
        {
            strcat(pszResult, pszReplace);
        }

        nOffset = pszBoundary - pszString + nFindLength;
    }

    strcat(pszResult, pszString + nOffset);

    *ppszResult = pszResult;
cleanup:
    return dwError;

error:
    if(ppszResult)
    {
        *ppszResult = NULL;
    }
    SAFE_FREE_MEMORY(pszResult);
    goto cleanup;
}
