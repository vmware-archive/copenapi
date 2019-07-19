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

void
show_util_help(
    )
{
    printf("usage: copenapi-cli [options] COMMAND [command options]\n");
    printf("\n");

    printf("options    [--apispec - specify path to apispec to load.]\n");
    printf("           [--baseurl - server url including port]\n");
    printf("           [-k --insecure - bypass certificate verification.]\n");
    printf("           [-n --netrc - read user/pass from .netrc file in user's home]\n");
    printf("           [-u --user - user name. prompts for password.]\n");
    printf("           [-v --verbose - print detailed debug output]\n");
    printf("           [-X --request - specify request command (GET,PUT,POST,DELETE,PATCH)]\n");
    printf("           [-h --help - print this message]\n");
    printf("\n");
    printf("\n");
    printf("To see a list of available modules or end points loaded from apispec,\n");
    printf("invoke without params or with just --apispec param.\n");
    printf("\n");
}

uint32_t
show_help(
    PCMD_ARGS pArgs,
    PREST_API_DEF pApiDef
    )
{
    uint32_t dwError = 0;
    int nHasModule = 0;
    int nCmdCount = 0;

    if(!pArgs || !pApiDef)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    nCmdCount = pArgs->nCmdCount;
    if(!nCmdCount)
    {
        dwError = show_modules(pApiDef);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        PREST_API_MODULE pModule = NULL;
        const char *pszModule = pArgs->ppszCmds[0];
        dwError = coapi_find_module_by_name(pszModule,
                                            pApiDef->pModules,
                                            &pModule);
        if(dwError == ENOENT)
        {
            dwError = 0;
        }
        BAIL_ON_ERROR(dwError);

        if(!pModule)
        {
            fprintf(stdout,
                    "Module " BOLD "%s " RESET " not found."
                    "Check your api spec\n\n",
                    pszModule);
            dwError = show_modules(pApiDef);
            BAIL_ON_ERROR(dwError);
        }
        else if(nCmdCount == 1)
        {
            dwError = show_module_commands(pModule);
            BAIL_ON_ERROR(dwError);
        }
        else if(nCmdCount > 1)
        {
            dwError = show_method(
                          pModule,
                          pArgs->ppszCmds[1]);
            BAIL_ON_ERROR(dwError);
        }
    }

cleanup:
    return dwError;

error:
    show_util_help();
    goto cleanup;
}

uint32_t
show_modules(
    PREST_API_DEF pApiDef
    )
{
    uint32_t dwError = 0;
    PREST_API_MODULE pModule = NULL;

    if(!pApiDef)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    if(pApiDef->nNoModules)
    {
        fprintf(stdout, "COpenAPI maps tags in api spec to modules.\n");
        fprintf(stdout, "Did not find tags for this api spec.\n");
        fprintf(stdout, "base path is mapped as the default module.\n");
    }
    else
    {
        fprintf(stdout, "The following modules are supported.\n");
    }

    pModule = pApiDef->pModules;
    while(pModule)
    {
        fprintf(stdout,
                " " BOLD "%-15s " RESET ": %-75s\n",
                pModule->pszName,
                pModule->pszDescription);
        pModule = pModule->pNext;
    }

    fprintf(stdout, "To get help on a module, do <module> --help.\n");
    fprintf(stdout,
            "To get help on a module's command, "
            "do <module> <command> --help.\n");

cleanup:
    return dwError;

error:
    goto cleanup;
}

uint32_t
show_module_commands(
    PREST_API_MODULE pModule
    )
{
    uint32_t dwError = 0;
    PREST_API_ENDPOINT pEndPoint = NULL;
    const char *pszAlternate = NULL;

    if(!pModule)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    fprintf(stdout,
            "Commands under module " BOLD "%s " RESET ": \n",
            pModule->pszName);

    pEndPoint = pModule->pEndPoints;
    for(; pEndPoint; pEndPoint = pEndPoint->pNext)
    {
        int i = 0;
        for(i = 0; i < METHOD_COUNT; ++i)
        {
            PREST_API_METHOD pMethod = pEndPoint->pMethods[i];
            if(pMethod)
            {
		pszAlternate = pMethod->pszOperationId ? pMethod->pszOperationId : pEndPoint->pszCommandName;
                fprintf(stdout,
                        "%-15s %-75s\n",
                        pEndPoint->pszCommandName[0] == '{' ? pszAlternate : pEndPoint->pszCommandName,
                        pMethod->pszSummary);
            }
        }
    }
    fprintf(stdout,
            "\nTo get help on a command, "
            "do %s <command> --help.\n\n", pModule->pszName);

cleanup:
    return dwError;

error:
    goto cleanup;
}

uint32_t
find_matching_endpoints(
    PREST_API_ENDPOINT pEndPoints,
    const char *pszCmd,
    PREST_API_ENDPOINT **pppMatchingEndPoints
    )
{
    uint32_t dwError = 0;
    int nEndPoints = 0;
    PREST_API_ENDPOINT *ppMatchingEndPoints = NULL;
    PREST_API_ENDPOINT pEndPoint = NULL;

    if(!pEndPoints || IsNullOrEmptyString(pszCmd))
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    for(pEndPoint = pEndPoints; pEndPoint; pEndPoint = pEndPoint->pNext)
    {
        char *pszFind = strstr(pEndPoint->pszActualName, pszCmd);
        //must match at the end
        if(pszFind && !strcasecmp(pszFind, pszCmd))
        {
            ++nEndPoints;
        }
    }

    if(!nEndPoints)
    {
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_allocate_memory(
                  sizeof(PREST_API_ENDPOINT) * (nEndPoints + 1),
                  (void **)&ppMatchingEndPoints);
    BAIL_ON_ERROR(dwError);

    nEndPoints = 0;
    for(pEndPoint = pEndPoints; pEndPoint; pEndPoint = pEndPoint->pNext)
    {
        char *pszFind = strstr(pEndPoint->pszActualName, pszCmd);
        //must match at the end
        if(pszFind && !strcasecmp(pszFind, pszCmd))
        {
            ppMatchingEndPoints[nEndPoints++] = pEndPoint;
        }
    }

    *pppMatchingEndPoints = ppMatchingEndPoints;

cleanup:
    return dwError;

error:
    if(pppMatchingEndPoints)
    {
        *pppMatchingEndPoints = NULL;
    }
    SAFE_FREE_MEMORY(ppMatchingEndPoints);
    goto cleanup;
}

uint32_t
show_method_details(
    PREST_API_METHOD pMethod,
    int nMethodIndex
    )
{
    uint32_t dwError = 0;
    char *ppszMethods[METHOD_COUNT] = {"get", "put", "post", "delete", "patch"};
    fprintf(stdout, "Method: %s\n", ppszMethods[nMethodIndex]);
    fprintf(stdout, "Summary : %s\n", pMethod->pszSummary);
    fprintf(stdout, "Description : %s\n", pMethod->pszDescription);
    if(!pMethod->pParams)
    {
        fprintf(stdout, "Params : None\n");
    }
    else
    {
        int nParam = 0;
        PREST_API_PARAM pParam = NULL;
        for(pParam = pMethod->pParams; pParam; pParam = pParam->pNext)
        {
            fprintf(stdout,
                    "Param%d : %s - %s\n",
                    ++nParam,
                    pParam->pszName,
                    pParam->nRequired ? "Required" : "Optional");
            if(pParam->nOptionCount)
            {
                int i = 0;
                fprintf(stdout, "Values: [");
                for(i = 0; i < pParam->nOptionCount; ++i)
                {
                    fprintf(stdout,
                            "%s%s",
                            pParam->ppszOptions[i],
                            i + 1 == pParam->nOptionCount ? "" : ", ");
                }
                fprintf(stdout, "]\n");
            }
        }
    }
    fprintf(stdout, "\n");

    return dwError;
}

uint32_t
show_endpoint_by_operation_id(
    PREST_API_ENDPOINT pEndPoints,
    const char *pszCmd
    )
{
    uint32_t dwError = 0;
    int nEndPoints = 0;
    PREST_API_ENDPOINT pEndPoint = NULL;
    PREST_API_METHOD pMethod = NULL;
    PREST_API_METHOD pMethodTemp = NULL;
    RESTMETHOD method = METHOD_GET;

    if(!pEndPoints || IsNullOrEmptyString(pszCmd))
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    for(; pEndPoints; pEndPoints = pEndPoints->pNext)
    {
	/* need only consider path substitutes at end for this */
	if (pEndPoints->pszCommandName[0] != '{')
            continue;
	for(method = METHOD_GET; method < METHOD_COUNT; method++)
	{
	    pMethodTemp = pEndPoints->pMethods[method];
	    if (pMethodTemp && pMethodTemp->pszOperationId)
	    {
                if(strcmp(pszCmd, pMethodTemp->pszOperationId) == 0)
		{
                    pEndPoint = pEndPoints;
		    pMethod = pMethodTemp;
		    break;
		}
	    }
	}
    }

    if(!pEndPoint || !pMethod)
    {
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }

    fprintf(stdout, "\n");
    fprintf(stdout, "Name : %s\n", pEndPoint->pszActualName);
    fprintf(stdout, "\n");

    dwError = show_method_details(pMethod, pMethod->nMethod);
    BAIL_ON_ERROR(dwError);
error:
    return dwError;
}

uint32_t
show_all_matching_methods(
    PREST_API_ENDPOINT pEndPoints,
    const char *pszMethod
    )
{
    uint32_t dwError = 0;
    int i = 0;
    PREST_API_ENDPOINT *ppMatchingEndPoints = NULL;

    dwError = find_matching_endpoints(
                  pEndPoints,
                  pszMethod,
                  &ppMatchingEndPoints);
    BAIL_ON_ERROR(dwError);

    for(i = 0; ppMatchingEndPoints[i]; ++i)
    {
        int nMethodIndex = 0;
        fprintf(stdout, "\n");
        fprintf(stdout, "Name : %s\n", ppMatchingEndPoints[i]->pszActualName);
        fprintf(stdout, "\n");
        for(nMethodIndex = 0; nMethodIndex < METHOD_COUNT; ++nMethodIndex)
        {
            PREST_API_METHOD pMethod = ppMatchingEndPoints[i]->pMethods[nMethodIndex];
            if(!pMethod)
            {
                continue;
            }
	    dwError = show_method_details(pMethod, nMethodIndex);
            BAIL_ON_ERROR(dwError);
	}
    }
error:
    SAFE_FREE_MEMORY(ppMatchingEndPoints);
    return dwError;
}

uint32_t
show_method(
    PREST_API_MODULE pModule,
    const char *pszMethod
    )
{
    uint32_t dwError = 0;
    PREST_API_ENDPOINT pEndPoint = NULL;
    PREST_API_METHOD pMethod = NULL;

    if(!pModule || IsNullOrEmptyString(pszMethod))
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = show_all_matching_methods(pModule->pEndPoints, pszMethod);
    if (dwError == ENOENT)
    {
        dwError = show_endpoint_by_operation_id(pModule->pEndPoints, pszMethod);
    }
    BAIL_ON_ERROR(dwError);

error:
    return dwError;
}
