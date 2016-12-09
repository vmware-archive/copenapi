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

static CMD_ARGS _main_opt = {0};

//options - 
static struct option _pstMainOptions[] =
{
    {"help",                no_argument, &_main_opt.nHelp, 'h'},
    {"verbose",             no_argument, &_main_opt.nVerbose, 'v'},
    {"apispec",             required_argument, 0, 0},
    {0, 0, 0, 0}
};

uint32_t
parse_main_args(
    int argc,
    char **argv,
    PCMD_ARGS *ppCmdArgs
    )
{
    uint32_t dwError = 0;
    PCMD_ARGS pCmdArgs = NULL;
    int nOptionIndex = 0;
    int nOption = 0;

    if(!argv || !ppCmdArgs)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_allocate_memory(
                            sizeof(CMD_ARGS),
                            (void**)&pCmdArgs);
    BAIL_ON_ERROR(dwError);

    opterr = 0;//tell getopt to not print errors
    while (1)
    {
        nOption = getopt_long (
                      argc,
                      argv,
                      "hv",
                      _pstMainOptions,
                      &nOptionIndex);
        if (nOption == -1)
            break;

        switch(nOption)
        {
            case 0:
                dwError = parse_option(
                              _pstMainOptions[nOptionIndex].name,
                              optarg,
                              pCmdArgs);
                BAIL_ON_ERROR(dwError);
                break;
            case 'h':
                _main_opt.nHelp = 1;
            break;
            case 'v':
                _main_opt.nVerbose = 1;
            break;
            case '?':
            break;
            default:
            break;
        }
    }

    pCmdArgs->nHelp = _main_opt.nHelp;
    pCmdArgs->nVerbose = _main_opt.nVerbose;

    dwError = collect_extra_args(optind,
                                 argc,
                                 argv,
                                 &pCmdArgs->ppszCmds,
                                 &pCmdArgs->nCmdCount);
    BAIL_ON_ERROR(dwError);

    if(pCmdArgs->nCmdCount < 1)
    {
        pCmdArgs->nHelp = 1;
    }

    *ppCmdArgs = pCmdArgs;

cleanup:
    return dwError;

error:
    if(ppCmdArgs)
    {
        *ppCmdArgs = NULL;
    }
    if(pCmdArgs)
    {
        free_cmd_args(pCmdArgs);
    }
    goto cleanup;
}

uint32_t
collect_extra_args(
    int argIndex,
    int argc,
    char* const* argv,
    char*** pppszCmds,
    int* pnCmdCount
    )
{
    uint32_t dwError = 0;
    char** ppszCmds = NULL;
    int nCmdCount = 0;

    if(!argv || !pppszCmds || !pnCmdCount)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    if (argIndex < argc)
    {
        int nIndex = 0;
        nCmdCount = argc - argIndex;
        dwError = coapi_allocate_memory(nCmdCount * sizeof(char*),
                                        (void**)&ppszCmds);
        BAIL_ON_ERROR(dwError);
        
        while (argIndex < argc)
        {
            dwError = coapi_allocate_string(argv[argIndex++],
                                            &ppszCmds[nIndex++]);
            BAIL_ON_ERROR(dwError);
        }
    }

    *pppszCmds = ppszCmds;
    *pnCmdCount = nCmdCount;

cleanup:
    return dwError;

error:
    if(pppszCmds)
    {
        *pppszCmds = NULL;
    }
    if(pnCmdCount)
    {
        *pnCmdCount = 0;
    }
    coapi_free_string_array_with_count(ppszCmds, nCmdCount);
    goto cleanup;
}

uint32_t
parse_option(
    const char* pszName,
    const char* pszArg,
    PCMD_ARGS pCmdArgs
    )
{
    uint32_t dwError = 0;

    if(IsNullOrEmptyString(pszName) ||
       !pCmdArgs)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    if(!strcasecmp(pszName, "apispec"))
    {
        dwError = coapi_allocate_string(
                      pszArg,
                      &pCmdArgs->pszApiSpec);
        BAIL_ON_ERROR(dwError);
    }
cleanup:
    return dwError;

error:
    goto cleanup;
}

void
free_cmd_args(
    PCMD_ARGS pCmdArgs
    )
{
    int nIndex = 0;
    if(pCmdArgs)
    {
        SAFE_FREE_MEMORY(pCmdArgs->pszApiSpec);
        coapi_free_string_array_with_count(pCmdArgs->ppszCmds,
                                           pCmdArgs->nCmdCount);
    }
    SAFE_FREE_MEMORY(pCmdArgs);
}
