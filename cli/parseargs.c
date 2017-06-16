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

static CMD_ARGS _main_opt = {0};

//options - 
static struct option _pstMainOptions[] =
{
    {OPT_HELP,     no_argument, &_main_opt.nHelp, 'h'},
    {OPT_VERBOSE,  no_argument, &_main_opt.nVerbose, 'v'},
    {OPT_INSECURE, no_argument, &_main_opt.nInsecure, 'k'},
    {OPT_APISPEC,  required_argument, 0, 'a'},
    {OPT_USER,     required_argument, 0, 'u'},
    {OPT_BASEURL,  required_argument, 0, 'b'},
    {OPT_NETRC,    no_argument, &_main_opt.nNetrc, 'n'},
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
                      "a:b:hknu:v",
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
            case 'a':
                dwError = parse_option(
                              OPT_APISPEC,
                              optarg,
                              pCmdArgs);
                BAIL_ON_ERROR(dwError);
            break;
            case 'b':
                dwError = parse_option(
                              OPT_BASEURL,
                              optarg,
                              pCmdArgs);
                BAIL_ON_ERROR(dwError);
            break;
            case 'h':
                _main_opt.nHelp = 1;
            break;
            case 'k':
                _main_opt.nInsecure = 1;
            break;
            case 'n':
                _main_opt.nNetrc = 1;
            break;
            case 'u':
                dwError = parse_option(
                              OPT_USER,
                              optarg,
                              pCmdArgs);
                BAIL_ON_ERROR(dwError);
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
    pCmdArgs->nInsecure = _main_opt.nInsecure;
    pCmdArgs->nVerbose = _main_opt.nVerbose;
    pCmdArgs->nNetrc = _main_opt.nNetrc;

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

    if(!strcasecmp(pszName, OPT_APISPEC))
    {
        dwError = coapi_allocate_string(
                      pszArg,
                      &pCmdArgs->pszApiSpec);
        BAIL_ON_ERROR(dwError);
    }
    else if(!strcasecmp(pszName, OPT_USER))
    {
        dwError = coapi_allocate_string(
                      pszArg,
                      &pCmdArgs->pszUser);
        BAIL_ON_ERROR(dwError);
    }
    else if(!strcasecmp(pszName, OPT_BASEURL))
    {
        dwError = coapi_allocate_string(
                      pszArg,
                      &pCmdArgs->pszBaseUrl);
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
        SAFE_FREE_MEMORY(pCmdArgs->pszUser);
        SAFE_FREE_MEMORY(pCmdArgs->pszUserPass);
        SAFE_FREE_MEMORY(pCmdArgs->pszBaseUrl);
        coapi_free_string_array_with_count(pCmdArgs->ppszCmds,
                                           pCmdArgs->nCmdCount);
    }
    SAFE_FREE_MEMORY(pCmdArgs);
}
