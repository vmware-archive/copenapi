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

    fprintf(stdout, "The following modules are supported.\n");

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
                fprintf(stdout,
                        "%-15s %-75s\n",
                        pEndPoint->pszCommandName,
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
        char *pszFind = strstr(pEndPoint->pszName, pszCmd);
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
        char *pszFind = strstr(pEndPoint->pszName, pszCmd);
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
show_method(
    PREST_API_MODULE pModule,
    const char *pszMethod
    )
{
    uint32_t dwError = 0;
    int i = 0;
    PREST_API_ENDPOINT *ppMatchingEndPoints = NULL;
    PREST_API_ENDPOINT *ppEndPoint = NULL;

    if(!pModule || IsNullOrEmptyString(pszMethod))
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = find_matching_endpoints(
                  pModule->pEndPoints,
                  pszMethod,
                  &ppMatchingEndPoints);
    BAIL_ON_ERROR(dwError);

    for(i = 0; ppMatchingEndPoints[i]; ++i)
    {
        int nMethodIndex = 0;
        fprintf(stdout, "\n");
        fprintf(stdout, "Name : %s\n", ppMatchingEndPoints[i]->pszName);
        fprintf(stdout, "\n");
        for(nMethodIndex = 0; nMethodIndex < METHOD_COUNT; ++nMethodIndex)
        {
            char *ppszMethods[METHOD_COUNT] =
                {"get", "put", "post", "delete", "patch"};
            PREST_API_METHOD pMethod = ppMatchingEndPoints[i]->pMethods[nMethodIndex];
            if(!pMethod)
            {
                continue;
            }
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
        }
    }

cleanup:
    SAFE_FREE_MEMORY(ppMatchingEndPoints);
    return dwError;

error:
    goto cleanup;
}
