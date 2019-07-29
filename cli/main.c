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
init_rest_cmd_args(
    PCMD_ARGS pArgs,
    PREST_API_DEF pApiDef,
    PREST_CMD_ARGS *ppRestArgs
    )
{
    uint32_t dwError = 0;
    PREST_CMD_ARGS pRestArgs = NULL;

    if(!pArgs || pArgs->nCmdCount < 2 || !pApiDef || !ppRestArgs)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_allocate_memory(
                  sizeof(REST_CMD_ARGS),
                  (void **)&pRestArgs);
    BAIL_ON_ERROR(dwError);

    pRestArgs->nRestMethod = pArgs->nRestMethod;

    dwError = coapi_allocate_string(
                  pArgs->ppszCmds[0],
                  &pRestArgs->pszModule);
    BAIL_ON_ERROR(dwError);

    dwError = coapi_allocate_string(
                  pArgs->ppszCmds[1],
                  &pRestArgs->pszCmd);
    BAIL_ON_ERROR(dwError);

    //Gather params for the command specified
    dwError = rest_get_cmd_params(pApiDef, pRestArgs);
    BAIL_ON_ERROR(dwError);

    *ppRestArgs = pRestArgs;
cleanup:
    return dwError;

error:
    if(ppRestArgs)
    {
        *ppRestArgs = NULL;
    }
    free_rest_cmd_args(pRestArgs);
    goto cleanup;
}

int
main(
    int argc,
    char **argv
    )
{
    uint32_t dwError = 0;
    PREST_API_DEF pApiDef = NULL;
    PREST_CMD_ARGS pRestCmdArgs = NULL;
    PCMD_ARGS pArgs = NULL;
    char **argvDup = NULL;
    char *pszPass = NULL;

    dwError = dup_argv(argc, argv, &argvDup);
    BAIL_ON_ERROR(dwError);

    dwError = parse_main_args(argc, argv, &pArgs);
    BAIL_ON_ERROR(dwError);

    if(argc == 2 && pArgs->nHelp)
    {
        show_util_help();
        goto cleanup;
    }

    dwError = get_config_data(&pArgs->pConfigData);
    BAIL_ON_ERROR(dwError);

    if(IsNullOrEmptyString(pArgs->pszApiSpec))
    {
        dwError = get_default_value(pArgs->pConfigData,
                                    CONFIG_KEY_APISPEC,
                                    &pArgs->pszApiSpec);
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_load_from_file(pArgs->pszApiSpec, &pApiDef);
    BAIL_ON_ERROR(dwError);

    if(argc < 2 || pArgs->nHelp)
    {
        show_help(pArgs, pApiDef);
        goto cleanup;
    }

    //Must have a module and command to proceed now
    if(pArgs->nCmdCount < 2)
    {
        show_help(pArgs, pApiDef);
        goto cleanup;
    }

    dwError = init_rest_cmd_args(pArgs, pApiDef, &pRestCmdArgs);
    BAIL_ON_ERROR(dwError);

    if(pRestCmdArgs->nParamCount > 0)
    {
        dwError = parse_cmd_args(argc, argvDup, pRestCmdArgs);
        BAIL_ON_ERROR(dwError);
    }

    if(IsNullOrEmptyString(pArgs->pszBaseUrl))
    {
        dwError = get_default_value(pArgs->pConfigData,
                                    CONFIG_KEY_BASEURL,
                                    &pArgs->pszBaseUrl);
        BAIL_ON_ERROR(dwError);
    }

    if (!pArgs->nInsecure)
    {
        dwError = get_default_int(pArgs->pConfigData,
                                    CONFIG_KEY_INSECURE,
                                    &pArgs->nInsecure);
        BAIL_ON_ERROR(dwError);
    }

    if(!pArgs->nNetrc && !IsNullOrEmptyString(pArgs->pszUser))
    {
        fprintf(stdout, "Password: ");

        dwError = read_password_no_echo(&pszPass);
        fprintf(stdout, "\n");
        BAIL_ON_ERROR(dwError);

        dwError = coapi_allocate_string_printf(
                      &pArgs->pszUserPass,
                      "%s:%s",
                      pArgs->pszUser,
                      pszPass);
        BAIL_ON_ERROR(dwError);
    }

    dwError = curl_global_init(CURL_GLOBAL_ALL);
    BAIL_ON_ERROR(dwError);

    dwError = rest_exec(pApiDef, pArgs, pRestCmdArgs);
    BAIL_ON_ERROR(dwError);

cleanup:
    SAFE_FREE_MEMORY(pszPass);
    curl_global_cleanup();
    if(argvDup)
    {
        coapi_free_string_array_with_count(argvDup, argc);
    }
    if(pArgs)
    {
        free_cmd_args(pArgs);
    }
    if(pApiDef)
    {
        coapi_free_api_def(pApiDef);
    }
    free_rest_cmd_args(pRestCmdArgs);
    return dwError;

error:
    if(pArgs && !pArgs->pszApiSpec)
    {
        fprintf(stderr,
"\nPlease specify an api spec using one or more of the following:\n"
"1. adding a file named .copenapi in your home or current directory\n"
"2. specify the rest api spec json file in command line using --apispec\n\n"
".copenapi file should have the following conf file format\n"
"[default]\n"
"apispec=/etc/pmd/restapispec.json\n\n"
"Search is done in the following order and stops on first find.\n"
"--apispec\n"
".copenapi in current working directory\n"
".copenapi in user's home directory\n\n");
    }

    goto cleanup;
}
