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
