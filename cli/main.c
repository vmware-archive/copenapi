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

int
main(
    int argc,
    char **argv
    )
{
    uint32_t dwError = 0;
    PREST_API_DEF pApiDef = NULL;
    PPARSE_CONTEXT pContext = NULL;
    PCMD_ARGS pArgs = NULL;
    char **argvDup = NULL;
    char *pszDefaultApiSpec = NULL;
    const char *pszApiSpec = NULL;
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

    pszApiSpec = pArgs->pszApiSpec;

    if(IsNullOrEmptyString(pszApiSpec))
    {
        dwError = get_default_api_spec(&pszDefaultApiSpec);
        BAIL_ON_ERROR(dwError);

        pszApiSpec = pszDefaultApiSpec;
    }

    dwError = coapi_load_from_file(pszApiSpec, &pApiDef);
    BAIL_ON_ERROR(dwError);

    if(argc < 2 || pArgs->nHelp)
    {
        show_help(pArgs, pApiDef);
        goto cleanup;
    }

    dwError = coapi_allocate_memory(sizeof(PARSE_CONTEXT),
                                    (void **)&pContext);
    BAIL_ON_ERROR(dwError);

    pContext->parseState = PARSE_STATE_BEGIN;

    dwError = params_parse(argc, (const char **)argvDup, pContext);
    BAIL_ON_ERROR(dwError);

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

    dwError = rest_exec(pApiDef, pArgs, pContext);
    BAIL_ON_ERROR(dwError);

cleanup:
    SAFE_FREE_MEMORY(pszPass);
    SAFE_FREE_MEMORY(pszDefaultApiSpec);
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
    free_parse_context(pContext);
    return dwError;

error:
    if(!pszApiSpec)
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
