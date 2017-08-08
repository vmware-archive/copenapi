/*
 * Copyright © 2016-2017 VMware, Inc.  All Rights Reserved.
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
parse_cmd_option(
    const char* pszName,
    const char* pszArg,
    PREST_CMD_ARGS pRestArgs
    )
{
    uint32_t dwError = 0;
    PREST_CMD_PARAM pParam = NULL;

    if(IsNullOrEmptyString(pszName) || !pRestArgs)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    for(pParam = pRestArgs->pParams; pParam; pParam = pParam->pNext)
    {
        if(!strcmp(pszName, pParam->pszName))
        {
            dwError = coapi_allocate_string(pszArg, &pParam->pszValue);
            BAIL_ON_ERROR(dwError);
            break;
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

uint32_t
populate_options(
    PREST_CMD_ARGS pRestArgs,
    struct option **ppOptions
    )
{
    uint32_t dwError = 0;
    struct option *pOptions = NULL;
    PREST_CMD_PARAM pParam = NULL;
    int i = 0;

    if(!pRestArgs || pRestArgs->nParamCount <= 0 || !ppOptions)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_allocate_memory(
                  sizeof(struct option) * (pRestArgs->nParamCount + 1),
                  (void **)&pOptions);
    BAIL_ON_ERROR(dwError);

    for(i = 0, pParam = pRestArgs->pParams;
        pParam;
        pParam = pParam->pNext, i++)
    {
        pOptions[i].name = pParam->pszName;
        pOptions[i].has_arg = required_argument;
    }

    *ppOptions = pOptions;
cleanup:
    return dwError;

error:
    if(ppOptions)
    {
        *ppOptions = NULL;
    }
    SAFE_FREE_MEMORY(pOptions);
    goto cleanup;
}

uint32_t
validate_options(
    PREST_CMD_ARGS pRestArgs
    )
{
    uint32_t dwError = 0;
    PREST_CMD_PARAM pParam = NULL;

    if(!pRestArgs)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    for(pParam = pRestArgs->pParams; pParam; pParam = pParam->pNext)
    {
        if(pParam->nRequired && IsNullOrEmptyString(pParam->pszValue))
        {
            fprintf(stderr, "Parameter %s is required\n", pParam->pszName);
            dwError = EINVAL;
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

uint32_t
parse_cmd_args(
    int argc,
    char **argv,
    PREST_CMD_ARGS pRestArgs
    )
{
    uint32_t dwError = 0;
    int nOptionIndex = 0;
    int nOption = 0;

    struct option *pOptions = NULL;

    if(!argv || !pRestArgs)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    //reset arg parse index for rescan
    optind = 1;

    opterr = 0;//tell getopt to not print errors

    dwError = populate_options(pRestArgs, &pOptions);
    BAIL_ON_ERROR(dwError);

    while (1)
    {
        nOption = getopt_long (
                      argc,
                      argv,
                      "",
                      pOptions,
                      &nOptionIndex);
        if (nOption == -1)
            break;

        switch(nOption)
        {
            case 0:
                dwError = parse_cmd_option(
                              pOptions[nOptionIndex].name,
                              optarg,
                              pRestArgs);
                BAIL_ON_ERROR(dwError);
                break;
            case '?':
            break;
            default:
            break;
        }
    }

    dwError = validate_options(pRestArgs);
    BAIL_ON_ERROR(dwError);

cleanup:
    return dwError;

error:
    SAFE_FREE_MEMORY(pOptions);
    goto cleanup;
}

uint32_t
get_param_by_name(
    PREST_CMD_ARGS pRestArgs,
    const char *pszName,
    PREST_CMD_PARAM *ppParam
    )
{
    uint32_t dwError = 0;
    PREST_CMD_PARAM pParam = NULL;

    if(!pRestArgs || IsNullOrEmptyString(pszName) || !ppParam)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    for(pParam = pRestArgs->pParams; pParam; pParam = pParam->pNext)
    {
        if(!strcasecmp(pParam->pszName, pszName))
        {
            break;
        }
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
free_rest_cmd_params(
    PREST_CMD_PARAM pParams
    )
{
    PREST_CMD_PARAM pTemp = NULL;
    if(!pParams)
    {
        return;
    }
    while(pParams)
    {
        pTemp = pParams->pNext;
        SAFE_FREE_MEMORY(pParams->pszName);
        SAFE_FREE_MEMORY(pParams->pszValue);
        SAFE_FREE_MEMORY(pParams);
        pParams = pTemp;
    }
}

void
free_rest_cmd_args(
    PREST_CMD_ARGS pRestArgs
    )
{
    if(!pRestArgs)
    {
        return;
    }
    free_rest_cmd_params(pRestArgs->pParams);
    SAFE_FREE_MEMORY(pRestArgs->pszModule);
    SAFE_FREE_MEMORY(pRestArgs->pszCmd);
    SAFE_FREE_MEMORY(pRestArgs);
}
