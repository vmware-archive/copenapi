/*
 * Copyright © 206-2017 VMware, Inc.  All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, without
 * warranties or conditions of any kind, EITHER EXPRESS OR IMPLIED.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#include "includes.h"

uint32_t
params_make_flag(
    const char *pszArg,
    PPARAM *ppParam
    )
{
    uint32_t dwError = 0;
    PPARAM pParam = NULL;

    if(IsNullOrEmptyString(pszArg) || pParam)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_allocate_memory(sizeof(PARAM), (void **)&pParam);
    BAIL_ON_ERROR(dwError);

    dwError = coapi_allocate_string(pszArg, &pParam->pszName);
    BAIL_ON_ERROR(dwError);

    *ppParam = pParam;

cleanup:
    return dwError;

error:
    if(ppParam)
    {
        *ppParam = NULL;
    }
    SAFE_FREE_MEMORY(pParam);
    goto cleanup;
}

uint32_t
params_parse_cb(
    const char *pszArg,
    PPARSE_CONTEXT pContext
    )
{
    uint32_t dwError = 0;
    int nFlag = 0;
    PPARAM pParam = NULL;
    PPARAM pParamLast = NULL;
    const char *pszFlag = NULL;

    if(IsNullOrEmptyString(pszArg) || !pContext)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pParamLast = pContext->pParams;
    while(pParamLast && pParamLast->pNext) pParamLast = pParamLast->pNext;

    dwError = param_is_flag(pszArg, &nFlag);
    BAIL_ON_ERROR(dwError);

    pszFlag = pszArg + 2;

    if(pContext->parseState == PARSE_STATE_BEGIN)
    {
        dwError = coapi_allocate_string(pszArg, &pContext->pszModule);
        BAIL_ON_ERROR(dwError);

        pContext->parseState = PARSE_STATE_READY;
    }
    else if(pContext->parseState == PARSE_STATE_READY)
    {
        if(nFlag)
        {
            dwError = params_make_flag(pszFlag, &pParam);
            BAIL_ON_ERROR(dwError);

            if(pParamLast)
            {
                pParamLast->pNext = pParam;
            }
            else
            {
                pContext->pParams = pParam;
            }
            pContext->parseState = PARSE_STATE_FLAG;
        }
        else
        {
            dwError = coapi_allocate_string(pszArg, &pContext->pszCmd);
            BAIL_ON_ERROR(dwError);
        }
    }
    else if(pContext->parseState == PARSE_STATE_FLAG)
    {
        if(nFlag)
        {
            dwError = params_make_flag(pszFlag, &pParam);
            BAIL_ON_ERROR(dwError);

            if(pParamLast)
            {
                pParamLast->pNext = pParam;
            }
            else
            {
                pContext->pParams = pParam;
            }
            pContext->parseState = PARSE_STATE_FLAG;
        }
        else
        {
            dwError = coapi_allocate_string(pszArg, &pParamLast->pszValue);
            BAIL_ON_ERROR(dwError);
            pContext->parseState = PARSE_STATE_READY;
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

uint32_t
params_get_bounded(
    char chBoundary,
    const char *pszStart,
    int *pnLength
    )
{
    uint32_t dwError = 0;
    int nLength = 0;
    char *pszEnd = NULL;
    if(IsNullOrEmptyString(pszStart) || !pnLength)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pszEnd = strchr(pszStart + 1, chBoundary);
    if(IsNullOrEmptyString(pszEnd))
    {
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }
    nLength = (pszEnd - pszStart) + 1;
    *pnLength = nLength;

cleanup:
    return dwError;
error:
    if(pnLength)
    {
        *pnLength = 0;
    }
    goto cleanup;
}

uint32_t
params_invoke_cb(
    const char *pszStart,
    int nLength,
    PPARSE_CONTEXT pContext
    )
{
    uint32_t dwError = 0;
    char *pszArg = NULL;

    if(IsNullOrEmptyString(pszStart) || nLength <= 0 || !pContext)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pszArg = (char *)calloc(nLength + 1, 1);
    strncpy(pszArg, pszStart, nLength);

    params_parse_cb(pszArg, pContext);

cleanup:
    SAFE_FREE_MEMORY(pszArg);
    return dwError;

error:
    goto cleanup;
}

uint32_t
params_parse_string(
    const char *pszString,
    PPARSE_CONTEXT pContext
    )
{
    uint32_t dwError = 0;
    char chSep = ' ';
    int nIndex = 0;
    char *pszBoundary = NULL;
    const char *pszStart = NULL;

    if(IsNullOrEmptyString(pszString))
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pszStart = pszString;
    while(1)
    {
        char *pszArg = NULL;
        int nLength = 0;
        pszBoundary = strchr(pszStart, chSep);

        if(!pszBoundary || pszBoundary <= pszStart)
        {
            break;
        }

        dwError = params_invoke_cb(pszStart, pszBoundary - pszStart, pContext);
        BAIL_ON_ERROR(dwError);

        while(*pszBoundary)
        {
            if(isspace(*pszBoundary))
            {
                ++pszBoundary;
            }
            else if(*pszBoundary == '"')
            {
                dwError = params_get_bounded('"', pszBoundary, &nLength);
                BAIL_ON_ERROR(dwError);

                dwError = params_invoke_cb(pszBoundary, nLength, pContext);
                BAIL_ON_ERROR(dwError);

                pszBoundary += nLength;
            }
            else
            {
                break;
            }
        }
        if(IsNullOrEmptyString(pszBoundary))
        {
            pszStart = NULL;
            break;
        }
        nIndex = pszBoundary - pszStart;
        pszStart += nIndex;
    }

    if(!IsNullOrEmptyString(pszStart))
    {
        params_parse_cb(pszStart, pContext);
    }

cleanup:
    return dwError = 0;

error:
    goto cleanup;
}

uint32_t
params_parse(
    int nArgc,
    const char **argv,
    PPARSE_CONTEXT pContext
    )
{
    uint32_t dwError = 0;
    int i = 0;

    if(nArgc < 2 || !argv || !pContext)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    for(i = 1; i < nArgc; ++i)
    {
        params_parse_cb(argv[i], pContext);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

uint32_t
param_is_flag(
    const char *pszArg,
    int *pnFlag
    )
{
    uint32_t dwError = 0;
    int nFlag = 0;
    int nLen = 0;
    char *pszStart = NULL;
    const char FLAG = '-';

    if(IsNullOrEmptyString(pszArg) || !pnFlag)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    nLen = strlen(pszArg);
    if(nLen < 2)
    {
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }

    if(pszArg[0] == FLAG)
    {
        int i = 1;
        if(pszArg[1] == FLAG)
        {
            i++;
        }
        if(isalnum(pszArg[i]))
        {
            nFlag = 1;
        }
    }

    *pnFlag = nFlag;

cleanup:
    return dwError;

error:
    if(dwError == ENOENT)
    {
        dwError = 0;
    }
    if(pnFlag)
    {
        *pnFlag = 0;
    }
    goto cleanup;
}

uint32_t
has_param(
    PPARSE_CONTEXT pContext,
    const char *pszName
    )
{
    PPARAM pParam = NULL;
    return get_param_by_name(pContext, pszName, &pParam);
}

uint32_t
get_param_by_name(
    PPARSE_CONTEXT pContext,
    const char *pszName,
    PPARAM *ppParam
    )
{
    uint32_t dwError = 0;
    PPARAM pParam = NULL;

    if(!pContext || IsNullOrEmptyString(pszName) || !ppParam)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pParam = pContext->pParams;
    while(pParam)
    {
        if(!strcasecmp(pParam->pszName, pszName))
        {
            break;
        }
        pParam = pParam->pNext;
    }

    if(!pParam)
    {
        dwError = ENOENT;
    }

    *ppParam = pParam;

cleanup:
    return dwError;

error:
    if(ppParam)
    {
        *ppParam = NULL;
    }
    goto cleanup;
}

void
print_context(
    PPARSE_CONTEXT pContext
    )
{
    PPARAM pParam = NULL;
    if(!pContext)
    {
        return;
    }
    pParam = pContext->pParams;
    printf("Module = %s, Command = %s\n",
           pContext->pszModule,
           pContext->pszCmd);
    for(; pParam; pParam = pParam->pNext)
    {
        printf("Name: %s, Value: %s\n",
               pParam->pszName,
               pParam->pszValue);
    }
}

void
free_parse_context(
    PPARSE_CONTEXT pContext
    )
{
    if(!pContext)
    {
        return;
    }
}
