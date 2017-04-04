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
coapi_load_modules(
    json_t *pRoot,
    PREST_API_MODULE *ppApiModules
    )
{
    uint32_t dwError = 0;
    json_t *pTags = NULL;
    json_t *pTag = NULL;
    int i = 0;
    PREST_API_MODULE pApiModules = NULL;
    PREST_API_MODULE pApiModule = NULL;

    if(!pRoot || !ppApiModules)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pTags = json_object_get(pRoot, "tags");
    if(!pTags)
    {
        dwError = ENODATA;
        BAIL_ON_ERROR(dwError);
    }

    json_array_foreach(pTags, i, pTag)
    {
        json_t *pDescription = NULL;
        json_t *pName = json_object_get(pTag, "name");
        if(!pName)
        {
            fprintf(stderr, "name not found for tag in api def\n");
            dwError = EINVAL;
            BAIL_ON_ERROR(dwError);
        }

        dwError = coapi_allocate_memory(sizeof(REST_API_MODULE),
                                        (void **)&pApiModule);
        BAIL_ON_ERROR(dwError);

        dwError = coapi_allocate_string(json_string_value(pName),
                                    &pApiModule->pszName);
        BAIL_ON_ERROR(dwError);

        dwError = json_safe_get_string_value(
                      pTag,
                      "description",
                      &pApiModule->pszDescription);
        BAIL_ON_ERROR(dwError);

        pApiModule->pNext = pApiModules;
        pApiModules = pApiModule;
        pApiModule = NULL;
    }

    *ppApiModules = pApiModules;

cleanup:
    return dwError;

error:
    if(ppApiModules)
    {
        *ppApiModules = NULL;
    }
    coapi_free_api_module(pApiModules);
    coapi_free_api_module(pApiModule);
    goto cleanup;
}

uint32_t
coapi_replace_endpoint_path(
    const char *pszActualName,
    PREST_API_PARAM pApiParams,
    char **ppszName
    )
{
    uint32_t dwError = 0;
    char *pszName = NULL;
    char *pszNameTemp = NULL;
    char *pszPath = NULL;
    PREST_API_PARAM pApiParam = NULL;

    if(IsNullOrEmptyString(pszActualName) || !ppszName)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    for(pApiParam = pApiParams; pApiParam; pApiParam = pApiParam->pNext)
    {
        if(strcmp(pApiParam->pszIn, "path"))
        {
            continue;
        }

        dwError = coapi_allocate_string_printf(
                      &pszPath,
                      "{%s}",
                      pApiParam->pszName);
        BAIL_ON_ERROR(dwError);

        dwError = string_replace(
                      pszActualName,
                      pszPath,
                      "*",
                      &pszNameTemp);
        if(dwError == ENOENT)
        {
            fprintf(stderr,
                    "path '%s' specified with bad path string: %s\n",
                    pApiParam->pszName,
                    pszActualName);
        }
        BAIL_ON_ERROR(dwError);

        SAFE_FREE_MEMORY(pszName);
        pszName = pszNameTemp;
        pszNameTemp = NULL;

        SAFE_FREE_MEMORY(pszPath);
        pszPath = NULL;
    }

    if(!pszName)
    {
        dwError = coapi_allocate_string(pszActualName, &pszName);
        BAIL_ON_ERROR(dwError);
    }

    *ppszName = pszName;
cleanup:
    SAFE_FREE_MEMORY(pszNameTemp);
    SAFE_FREE_MEMORY(pszPath);
    return dwError;

error:
    if(ppszName)
    {
        *ppszName = NULL;
    }
    SAFE_FREE_MEMORY(pszName);
    goto cleanup;
}

uint32_t
coapi_load_endpoints(
    json_t *pRoot,
    const char *pszBasePath,
    PREST_API_MODULE pApiModules
    )
{
    uint32_t dwError = 0;
    json_t *pPaths = NULL;
    json_t *pPath = NULL;
    const char *pszKey = NULL;
    PREST_API_ENDPOINT pEndPoint = NULL;
    PREST_API_METHOD pRestMethod = NULL;

    if(!pRoot || !pszBasePath || !pApiModules)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pPaths = json_object_get(pRoot, "paths");
    if(!pPaths)
    {
        fprintf(stderr, "paths not found in api def\n");
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    json_object_foreach(pPaths, pszKey, pPath)
    {
        const char *pszMethod = NULL;
        json_t *pMethod = NULL;
        PREST_API_MODULE pModule = NULL;
        const char *pszCmdStart = NULL;

        dwError = coapi_allocate_memory(sizeof(REST_API_ENDPOINT),
                                        (void **)&pEndPoint);
        BAIL_ON_ERROR(dwError);

        pszCmdStart = strrchr(pszKey, URL_SEPARATOR);
        pszCmdStart = pszCmdStart ? pszCmdStart + 1 : pszKey;

        dwError = coapi_allocate_string(pszCmdStart,
                                        &pEndPoint->pszCommandName);
        BAIL_ON_ERROR(dwError);

        dwError = coapi_allocate_string_printf(&pEndPoint->pszActualName,
                                               "%s%s",
                                               pszBasePath,
                                               pszKey);
        BAIL_ON_ERROR(dwError);

        json_object_foreach(pPath, pszMethod, pMethod)
        {
            RESTMETHOD nMethod = METHOD_INVALID;
            pRestMethod = NULL;

            dwError = coapi_get_rest_method(pszMethod, &nMethod);
            BAIL_ON_ERROR(dwError);

            if(pEndPoint->pMethods[nMethod])
            {
                printf("error entry already exists\n");
                dwError = EEXIST;
                BAIL_ON_ERROR(dwError);
            }

            dwError = coapi_allocate_memory(sizeof(REST_API_METHOD),
                                            (void **)&pRestMethod);
            BAIL_ON_ERROR(dwError);

            dwError = coapi_allocate_string(
                          pszMethod,
                          &pRestMethod->pszMethod);
            BAIL_ON_ERROR(dwError);

            dwError = json_safe_get_string_value(
                          pMethod,
                          "summary",
                          &pRestMethod->pszSummary);
            BAIL_ON_ERROR(dwError);

            dwError = json_safe_get_string_value(
                          pMethod,
                          "description",
                          &pRestMethod->pszDescription);
            BAIL_ON_ERROR(dwError);

            pRestMethod->nMethod = nMethod;

            dwError = coapi_load_parameters(pMethod, &pRestMethod->pParams);
            if(dwError == ENODATA)
            {
                dwError = 0;//allow no params
            }
            BAIL_ON_ERROR(dwError);

            if(IsNullOrEmptyString(pEndPoint->pszName))
            {
                dwError = coapi_replace_endpoint_path(
                              pEndPoint->pszActualName,
                              pRestMethod->pParams,
                              &pEndPoint->pszName);
                BAIL_ON_ERROR(dwError);

                pEndPoint->nHasPathSubs = strcmp(pEndPoint->pszActualName,
                                                 pEndPoint->pszName) != 0;
            }

            pEndPoint->pMethods[nMethod] = pRestMethod;
            pRestMethod = NULL;

            if(!pModule)
            {
                //find the module tagged
                dwError = coapi_find_tagged_module(pMethod, pApiModules, &pModule);
                if(dwError == ENODATA)
                {
                    pModule = pApiModules;
                    dwError = 0;
                }
                BAIL_ON_ERROR(dwError);
            }
        }

        dwError = coapi_module_add_endpoint(pModule, pEndPoint);
        BAIL_ON_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    coapi_free_api_endpoint(pEndPoint);
    coapi_free_api_method(pRestMethod);
    goto cleanup;
}

uint32_t
coapi_fill_enum(
    json_t *pJsonEnum,
    int *pnOptionCount,
    char ***pppszOptions
    )
{
    uint32_t dwError = 0;
    int i = 0;
    int nOptionCount = 0;
    json_t *pEnumValue = NULL;
    char **ppszOptions = NULL;

    if(!pJsonEnum || !pppszOptions || !pnOptionCount)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    nOptionCount = json_array_size(pJsonEnum);
    if(nOptionCount <= 0)
    {
        dwError = ENODATA;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_allocate_memory(sizeof(char *) * nOptionCount,
                                    (void **)&ppszOptions);
    BAIL_ON_ERROR(dwError);

    json_array_foreach(pJsonEnum, i, pEnumValue)
    {
        dwError = coapi_allocate_string(
                      json_string_value(pEnumValue),
                      &ppszOptions[i]);
        BAIL_ON_ERROR(dwError);
    }

    *pnOptionCount = nOptionCount;
    *pppszOptions = ppszOptions;
cleanup:
    return dwError;

error:
    if(dwError == ENODATA)
    {
        dwError = 0;
    }
    if(pppszOptions)
    {
        *pppszOptions = NULL;
    }
    if(pnOptionCount)
    {
        *pnOptionCount = 0;
    }
    coapi_free_string_array_with_count(ppszOptions, nOptionCount);
    goto cleanup;
}

uint32_t
coapi_load_parameters(
    json_t *pMethod,
    PREST_API_PARAM *ppParams
    )
{
    uint32_t dwError = 0;
    int i = 0;
    PREST_API_PARAM pParams = NULL;
    PREST_API_PARAM pParam = NULL;
    json_t *pJsonParams = NULL;
    json_t *pJsonParam = NULL;

    if(!pMethod || !ppParams)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pJsonParams = json_object_get(pMethod, "parameters");
    if(!pJsonParams)
    {
        dwError = ENODATA;
        BAIL_ON_ERROR(dwError);
    }

    json_array_foreach(pJsonParams, i, pJsonParam)
    {
        json_t *pTemp = NULL;
        dwError = coapi_allocate_memory(sizeof(REST_API_PARAM),
                                        (void **)&pParam);
        BAIL_ON_ERROR(dwError);

        pTemp = json_object_get(pJsonParam, "name");
        if(pTemp)
        {
            dwError = coapi_allocate_string(json_string_value(pTemp),
                                            &pParam->pszName);
            BAIL_ON_ERROR(dwError);
        }
        else
        {
            fprintf(stderr, "parameter: missing required field - name\n");
            dwError = EINVAL;
            BAIL_ON_ERROR(dwError);
        }

        pTemp = json_object_get(pJsonParam, "in");
        if(pTemp)
        {
            dwError = coapi_allocate_string(json_string_value(pTemp),
                                            &pParam->pszIn);
            BAIL_ON_ERROR(dwError);
        }
        else
        {
            fprintf(stderr, "parameter: missing required field - in\n");
            dwError = EINVAL;
            BAIL_ON_ERROR(dwError);
        }

        pTemp = json_object_get(pJsonParam, "required");
        if(pTemp)
        {
            pParam->nRequired = json_is_true(pTemp);
        }

        pTemp = json_object_get(pJsonParam, "type");
        if(pTemp)
        {
            dwError = coapi_get_rest_type(json_string_value(pTemp), &pParam->nType);
            BAIL_ON_ERROR(dwError);
        }

        pTemp = json_object_get(pJsonParam, "enum");
        if(pTemp)
        {
            dwError = coapi_fill_enum(pTemp,
                                      &pParam->nOptionCount,
                                      &pParam->ppszOptions);
            BAIL_ON_ERROR(dwError);
        }
        pParam->pNext = pParams;
        pParams = pParam;
        pParam = NULL;
    }

    *ppParams = pParams;

cleanup:
    return dwError;

error:
    if(ppParams)
    {
        *ppParams = NULL;
    }
    coapi_free_api_param(pParams);
    coapi_free_api_param(pParam);
    goto cleanup;
}

uint32_t
coapi_load_secure_scheme(
    json_t *pRoot,
    int *pnHasSecureScheme
    )
{
    uint32_t dwError = 0;
    json_t *pSchemes = NULL;
    json_t *pScheme = NULL;
    int i = 0;
    int nHasSecureScheme = 0;

    if(!pRoot || !pnHasSecureScheme)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pSchemes = json_object_get(pRoot, "schemes");
    if(!pSchemes)
    {
        dwError = ENODATA;
        BAIL_ON_ERROR(dwError);
    }

    if(!json_is_array(pSchemes))
    {
        fprintf(stderr, "schemes is not a json array\n");
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    json_array_foreach(pSchemes, i, pScheme)
    {
        const char *pszScheme = json_string_value(pScheme);
        if(IsNullOrEmptyString(pszScheme))
        {
            dwError = EINVAL;
            BAIL_ON_ERROR(dwError);
        }
        if(!strcasecmp("https", pszScheme))
        {
            nHasSecureScheme = 1;
            break;
        }
    }

    *pnHasSecureScheme = nHasSecureScheme;
cleanup:
    return dwError;

error:
    if(pnHasSecureScheme)
    {
        *pnHasSecureScheme = 0;
    }
    goto cleanup;
}

uint32_t
coapi_module_add_endpoint(
    PREST_API_MODULE pModule,
    PREST_API_ENDPOINT pEndPoint
    )
{
    uint32_t dwError = 0;
    PREST_API_ENDPOINT pEndPoints = NULL;

    if(!pModule || !pEndPoint)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pEndPoints = pModule->pEndPoints;
    while(pEndPoints && pEndPoints->pNext)
    {
        pEndPoints = pEndPoints->pNext;
    }
    if(pEndPoints)
    {
        pEndPoints->pNext = pEndPoint;
    }
    else
    {
        pModule->pEndPoints = pEndPoint;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

uint32_t
coapi_find_tagged_module(
    json_t *pPath,
    PREST_API_MODULE pModules,
    PREST_API_MODULE *ppModule
    )
{
    uint32_t dwError = 0;
    int nTagEntries = 0;
    PREST_API_MODULE pModule = NULL;
    PREST_API_MODULE pModuleTemp = NULL;
    json_t *pTags = NULL;
    json_t *pTag = NULL;
    const char *pszTag = NULL;

    if(!pPath || !pModules || !ppModule)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pTags = json_object_get(pPath, "tags");
    if(!pTags)
    {
        dwError = ENODATA;
        BAIL_ON_ERROR(dwError);
    }

    if(!json_is_array(pTags))
    {
        fprintf(stderr, "tags is not a json array\n");
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    nTagEntries = json_array_size(pTags);
    if(nTagEntries < 1)
    {
        fprintf(stderr, "there are no tag entries for this end point\n");
        dwError = ENODATA;
        BAIL_ON_ERROR(dwError);
    }
    else if(nTagEntries > 1)
    {
        fprintf(stdout, "there are more than one tag entries. using first entry\n");
    }

    pTag = json_array_get(pTags, 0);
    if(!pTag)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pszTag = json_string_value(pTag);
    if(!pszTag)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_find_module_by_name(pszTag, pModules, &pModule);
    BAIL_ON_ERROR(dwError);

    *ppModule = pModule;
cleanup:
    return dwError;

error:
    if(ppModule)
    {
        *ppModule = NULL;
    }
    goto cleanup;
}

uint32_t
coapi_find_module_by_name(
    const char *pszName,
    PREST_API_MODULE pModules,
    PREST_API_MODULE *ppModule
    )
{
    uint32_t dwError = 0;
    PREST_API_MODULE pModule = NULL;

    if(!pszName || !pModules || !ppModule)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    while(pModules)
    {
        if(!strcasecmp(pModules->pszName, pszName))
        {
            pModule = pModules;
            break;
        }
        pModules = pModules->pNext;
    }

    if(!pModule)
    {
        dwError = ENODATA;
        BAIL_ON_ERROR(dwError);
    }

    *ppModule = pModule;
cleanup:
    return dwError;

error:
    if(ppModule)
    {
        *ppModule = NULL;
    }
    goto cleanup;
}

uint32_t
coapi_add_default_module(
    const char *pszModuleName,
    PREST_API_MODULE *ppApiModules
    )
{
    uint32_t dwError = 0;
    PREST_API_MODULE pApiModule = NULL;

    if(IsNullOrEmptyString(pszModuleName) || !ppApiModules)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_allocate_memory(sizeof(REST_API_MODULE),
                                        (void **)&pApiModule);
    BAIL_ON_ERROR(dwError);

    dwError = coapi_allocate_string(pszModuleName, &pApiModule->pszName);
    BAIL_ON_ERROR(dwError);
    dwError = coapi_allocate_string(
                      "default module",
                      &pApiModule->pszDescription);
    BAIL_ON_ERROR(dwError);

    *ppApiModules = pApiModule;

cleanup:
    return dwError;

error:
    if(ppApiModules)
    {
        *ppApiModules = NULL;
    }
    coapi_free_api_module(pApiModule);
    goto cleanup;
}

uint32_t
coapi_find_endpoint_by_name(
    const char *pszName,
    PREST_API_ENDPOINT pEndPoints,
    PREST_API_ENDPOINT *ppEndPoint
    )
{
    uint32_t dwError = 0;
    PREST_API_ENDPOINT pEndPoint = NULL;

    if(!pszName || !pEndPoints || !ppEndPoint)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    while(pEndPoints)
    {
        if(!strcasecmp(pEndPoints->pszName, pszName))
        {
            pEndPoint = pEndPoints;
            break;
        }
        if(pEndPoints->nHasPathSubs)
        {
            //Maybe provide config to match with FNM_PATHNAME
            //cant make it default because
            //{path} to /path1/path2 will not match because of the "/"
            if(!fnmatch(pEndPoints->pszName, pszName, 0))
            {
                pEndPoint = pEndPoints;
                break;
            }
        }
        pEndPoints = pEndPoints->pNext;
    }

    if(!pEndPoint)
    {
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }

    *ppEndPoint = pEndPoint;
cleanup:
    return dwError;

error:
    if(ppEndPoint)
    {
        *ppEndPoint = NULL;
    }
    goto cleanup;
}

uint32_t
coapi_find_module_impl_by_name(
    const char *pszName,
    PREST_MODULE pModules,
    PREST_MODULE *ppModule
    )
{
    uint32_t dwError = 0;
    PREST_MODULE pModule = NULL;

    if(!pszName || !pModules || !ppModule)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    while(pModules)
    {
        if(!strcasecmp(pszName, pModules->pszEndPoint))
        {
            pModule = pModules;
            break;
        }
        ++pModules;
    }

    if(!pModule)
    {
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }

    *ppModule = pModule;
cleanup:
    return dwError;

error:
    if(ppModule)
    {
        *ppModule = NULL;
    }
    goto cleanup;
}

uint32_t
coapi_is_integer(
    const char *pszValue,
    int *pnValid
    )
{
    uint32_t dwError = 0;
    if(!pszValue)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    while(*pszValue)
    {
        if(!isdigit(*pszValue))
        {
            dwError = EINVAL;
            BAIL_ON_ERROR(dwError);
        }
        ++pszValue;
    }

    *pnValid = 1;

cleanup:
    return dwError;
error:
    if(pnValid)
    {
        *pnValid = 0;
    }
    goto cleanup;
}

uint32_t
coapi_check_param(
    PREST_API_PARAM pParam,
    const char *pszValue,
    int *pnValid
    )
{
    uint32_t dwError = 0;
    int nValid = 0;

    if(!pParam || !pszValue || !pnValid)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    switch(pParam->nType)
    {
        case RESTPARAM_ARRAY://Arrays are comma(%2C) separated strings
        case RESTPARAM_STRING:
        break;
        case RESTPARAM_INTEGER:
            dwError = coapi_is_integer(pszValue, &nValid);
            BAIL_ON_ERROR(dwError);
        break;
        default:
            nValid = 0;
    }

    *pnValid = nValid;
cleanup:
    return dwError;

error:
    if(pnValid)
    {
        *pnValid = 0;
    }
    goto cleanup;
}

uint32_t
coapi_get_required_params(
    PREST_API_METHOD pMethod,
    PREST_API_PARAM **pppRequiredParams,
    int *pnRequiredParamsCount
    )
{
    uint32_t dwError = 0;
    int nRequired = 0;
    int nIndex = 0;
    PREST_API_PARAM *ppRequired = NULL;
    PREST_API_PARAM *ppRequiredTemp = NULL;
    PREST_API_PARAM pParam = NULL;

    if(!pMethod || !pMethod->pParams || !pppRequiredParams || !pnRequiredParamsCount)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    for(pParam = pMethod->pParams; pParam; pParam = pParam->pNext)
    {
        if(pParam->nRequired)
        {
            nRequired++;
        }
    }

    if(!nRequired)
    {
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_allocate_memory(sizeof(PREST_API_PARAM)*(nRequired + 1),
                                    (void **)&ppRequired);
    BAIL_ON_ERROR(dwError);

    ppRequiredTemp = ppRequired;
    for(pParam = pMethod->pParams; pParam; pParam = pParam->pNext)
    {
        if(pParam->nRequired)
        {
            *ppRequiredTemp = pParam;
            ++ppRequiredTemp;
        }
    }

    *pppRequiredParams = ppRequired;
    *pnRequiredParamsCount = nRequired;

cleanup:
    return dwError;

error:
    if(dwError == ENOENT)
    {
        dwError = 0;
    }
    if(pppRequiredParams)
    {
        *pppRequiredParams = NULL;
    }
    if(pnRequiredParamsCount)
    {
        *pnRequiredParamsCount = 0;
    }
    SAFE_FREE_MEMORY(ppRequired);
    goto cleanup;
}

uint32_t
coapi_find_handler(
    PREST_API_DEF pApiDef,
    const char *pszEndPoint,
    const char *pszMethod,
    PREST_API_METHOD *ppMethod
    )
{
    uint32_t dwError = 0;
    PREST_API_METHOD pMethod = NULL;

    dwError = coapi_find_method(
                  pApiDef,
                  pszEndPoint,
                  pszMethod,
                  &pMethod);
    if(!dwError)
    {
        if(pMethod && pMethod->pFnImpl)
        {
            *ppMethod = pMethod;
        }
        else
        {
            dwError = ENOENT;
        }
    }
    return dwError;
}

uint32_t
coapi_find_method(
    PREST_API_DEF pApiDef,
    const char *pszEndPoint,
    const char *pszMethod,
    PREST_API_METHOD *ppMethod
    )
{
    uint32_t dwError = 0;
    PREST_API_MODULE pModule = NULL;
    RESTMETHOD nMethod = METHOD_INVALID;
    PREST_API_ENDPOINT pEndPoint = NULL;
    PREST_API_METHOD pMethod = NULL;
    if(!pApiDef || !pszEndPoint || !pszMethod || !ppMethod)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pModule = pApiDef->pModules;
    while(pModule)
    {
        dwError = coapi_find_endpoint_by_name(pszEndPoint,
                                        pModule->pEndPoints,
                                        &pEndPoint);
        if(dwError == ENOENT)
        {
            dwError = 0;
        }
        BAIL_ON_ERROR(dwError);

        if(pEndPoint)
        {
            break;
        }
        pModule = pModule->pNext;
    }

    if(!pEndPoint)
    {
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_get_rest_method(pszMethod, &nMethod);
    BAIL_ON_ERROR(dwError);

    pMethod = pEndPoint->pMethods[nMethod];

    if(!pMethod)
    {
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }

    *ppMethod = pMethod;
cleanup:
    return dwError;

error:
    if(ppMethod)
    {
        *ppMethod = NULL;
    }
    goto cleanup;
}

uint32_t
coapi_get_rest_type(
    const char *pszType,
    RESTPARAMTYPE *pnType
    )
{
    uint32_t dwError = 0;
    RESTPARAMTYPE nType = RESTPARAM_INVALID;

    if(!pszType || !pnType)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    if(!strcasecmp(pszType, "integer"))
    {
        nType = RESTPARAM_INTEGER;
    }
    else if(!strcasecmp(pszType, "number"))
    {
        nType = RESTPARAM_NUMBER;
    }
    else if(!strcasecmp(pszType, "string"))
    {
        nType = RESTPARAM_STRING;
    }
    else if(!strcasecmp(pszType, "boolean"))
    {
        nType = RESTPARAM_BOOLEAN;
    }
    else if(!strcasecmp(pszType, "array"))
    {
        nType = RESTPARAM_ARRAY;
    }
    else if(!strcasecmp(pszType, "file"))
    {
        nType = RESTPARAM_BOOLEAN;
    }
    else
    {
        fprintf(stderr, "type %s is not a valid parameter type\n", pszType);
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }

    *pnType = nType;

cleanup:
    return dwError;

error:
    if(pnType)
    {
        *pnType = RESTPARAM_INVALID;
    }
    goto cleanup;
}

uint32_t
coapi_get_rest_method_string(
    RESTMETHOD nMethod,
    char **ppszMethod
    )
{
    uint32_t dwError = 0;
    const char *pszTemp = NULL;
    char *pszMethod = NULL;

    if(nMethod < 0 || !ppszMethod)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    switch(nMethod)
    {
        case METHOD_GET:
            pszTemp = "get";
        break;
        case METHOD_PUT:
            pszTemp = "put";
        break;
        case METHOD_POST:
            pszTemp = "post";
        break;
        case METHOD_DELETE:
            pszTemp = "delete";
        break;
        case METHOD_PATCH:
            pszTemp = "patch";
        break;
        default:
            dwError = ENOENT;
            BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_allocate_string(pszTemp, &pszMethod);
    BAIL_ON_ERROR(dwError);

    *ppszMethod = pszMethod;
cleanup:
    return dwError;

error:
    if(ppszMethod)
    {
        *ppszMethod = NULL;
    }
    goto cleanup;
}

uint32_t
coapi_get_rest_method(
    const char *pszMethod,
    RESTMETHOD *pnMethod
    )
{
    uint32_t dwError = 0;
    RESTMETHOD nMethod = METHOD_INVALID;

    if(!pszMethod || !pnMethod)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    if(!strcasecmp(pszMethod, "get"))
    {
        nMethod = METHOD_GET;
    }
    else if(!strcasecmp(pszMethod, "patch"))
    {
        nMethod = METHOD_PATCH;
    }
    else if(!strcasecmp(pszMethod, "post"))
    {
        nMethod = METHOD_POST;
    }
    else if(!strcasecmp(pszMethod, "put"))
    {
        nMethod = METHOD_PUT;
    }
    else if(!strcasecmp(pszMethod, "delete"))
    {
        nMethod = METHOD_DELETE;
    }
    else
    {
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }

    *pnMethod = nMethod;

cleanup:
    return dwError;

error:
    if(pnMethod)
    {
        *pnMethod = METHOD_INVALID;
    }
    goto cleanup;
}

void
coapi_print_api_def(
    PREST_API_DEF pApiDef
    )
{
    PREST_API_MODULE pModules = NULL;
    PREST_API_ENDPOINT pEndPoint = NULL;

    pModules = pApiDef->pModules;
    while(pModules)
    {
        printf("Module = %s\n", pModules->pszName);
        pEndPoint = pModules->pEndPoints;
        while(pEndPoint)
        {
            if(pEndPoint->nHasPathSubs)
            {
                printf("\tEndPoint Actual = %s\n", pEndPoint->pszActualName);
                printf("\tEndPoint Registered = %s\n", pEndPoint->pszName);
            }
            else
            {
                printf("\tEndPoint = %s\n", pEndPoint->pszActualName);
            }
            pEndPoint = pEndPoint->pNext;
        }
        pModules = pModules->pNext;
    }
}

uint32_t
coapi_map_api_impl(
    PREST_API_DEF pApiDef,
    PMODULE_REG_MAP pRegMap
    )
{
    uint32_t dwError = 0;

    if(!pApiDef || !pRegMap)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    for(;pRegMap && pRegMap->pszName; ++pRegMap)
    {
        PREST_API_MODULE pModule = NULL;
        PREST_MODULE pModuleImpl = NULL;

        dwError = coapi_find_module_by_name(pRegMap->pszName,
                                      pApiDef->pModules,
                                      &pModule);
        if(dwError == ENODATA)
        {
            fprintf(stdout, "No api spec for module: %s\n", pRegMap->pszName);
            dwError = 0;
            continue;
        }
        BAIL_ON_ERROR(dwError);

        dwError = pRegMap->pFnModuleReg(&pModuleImpl);
        BAIL_ON_ERROR(dwError);

        dwError = coapi_map_module_impl(pModule, pModuleImpl);
        BAIL_ON_ERROR(dwError);
    }
cleanup:
    return dwError;

error:
    goto cleanup;
}

uint32_t
coapi_map_module_impl(
    PREST_API_MODULE pModule,
    PREST_MODULE pModuleImpl
    )
{
    uint32_t dwError = 0;
    PREST_API_ENDPOINT pEndPoint = NULL;

    if(!pModule || !pModuleImpl)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    for(; pModuleImpl && pModuleImpl->pszEndPoint; ++pModuleImpl)
    {
        RESTMETHOD nMethod = METHOD_COUNT;
        dwError = coapi_find_endpoint_by_name(pModuleImpl->pszEndPoint,
                                        pModule->pEndPoints,
                                        &pEndPoint);
        if(dwError == ENOENT)
        {
            dwError = 0;
            fprintf(stdout, "no api spec for %s\n", pModuleImpl->pszEndPoint);
            continue;
        }
        BAIL_ON_ERROR(dwError);

        while(nMethod--)
        {
            PREST_API_METHOD pMethodDef = pEndPoint->pMethods[nMethod];
            PFN_MODULE_ENDPOINT_CB pMethodImpl =
                pModuleImpl->pFnEndPointMethods[nMethod];

            if(!pMethodDef && !pMethodImpl)
            {
                continue;//no definition or implementation
            }
            if(pMethodDef && !pMethodImpl)
            {
                fprintf(stderr,
                        "no %d impl for defined %s\n", nMethod,
                        pEndPoint->pszName);
                continue;
            }
            if(!pMethodDef && pMethodImpl)
            {
                fprintf(stderr,
                        "no %d definition for impl %s\n", nMethod,
                        pModuleImpl->pszEndPoint);
                continue;
            }
            fprintf(stdout,
                    "mapping %d implementation for %s\n", nMethod,
                    pEndPoint->pszName);
            pMethodDef->pFnImpl = pMethodImpl;
        }
    }
cleanup:
    return dwError;

error:
    goto cleanup;
}

void
coapi_free_api_param(
    PREST_API_PARAM pParam
    )
{
    if(!pParam)
    {
        return;
    }
    while(pParam)
    {
        PREST_API_PARAM pParamTemp = pParam->pNext;

        SAFE_FREE_MEMORY(pParam->pszName);
        SAFE_FREE_MEMORY(pParam->pszIn);
        coapi_free_string_array_with_count(
            pParam->ppszOptions,
            pParam->nOptionCount);
        SAFE_FREE_MEMORY(pParam);

        pParam = pParamTemp;
    }
}

void
coapi_free_api_method(
    PREST_API_METHOD pMethod
    )
{
    if(!pMethod)
    {
        return;
    }
    coapi_free_api_param(pMethod->pParams);
    coapi_free_memory(pMethod->pszMethod);
    SAFE_FREE_MEMORY(pMethod->pszSummary);
    SAFE_FREE_MEMORY(pMethod->pszDescription);
    SAFE_FREE_MEMORY(pMethod);
}

void
coapi_free_api_endpoint(
    PREST_API_ENDPOINT pEndPoint
    )
{
    PREST_API_ENDPOINT pEndPointTemp = pEndPoint;
    if(!pEndPoint)
    {
        return;
    }
    while(pEndPoint)
    {
        int i = 0;
        for(; i < METHOD_COUNT; ++i)
        {
            if(pEndPoint->pMethods[i])
            {
                coapi_free_api_method(pEndPoint->pMethods[i]);
            }
        }
        SAFE_FREE_MEMORY(pEndPoint->pszActualName);
        SAFE_FREE_MEMORY(pEndPoint->pszName);
        SAFE_FREE_MEMORY(pEndPoint->pszCommandName);
        pEndPoint = pEndPoint->pNext;
        SAFE_FREE_MEMORY(pEndPointTemp);
        pEndPointTemp = pEndPoint;
    }
}

void
coapi_free_api_module(
    PREST_API_MODULE pModule
    )
{
    PREST_API_MODULE pModuleTemp = pModule;
    if(!pModule)
    {
        return;
    }
    while(pModule)
    {
        coapi_free_api_endpoint(pModule->pEndPoints);
        SAFE_FREE_MEMORY(pModule->pszName);
        SAFE_FREE_MEMORY(pModule->pszDescription);
        pModule = pModule->pNext;
        SAFE_FREE_MEMORY(pModuleTemp);
        pModuleTemp = pModule;
    }
}

void
coapi_free_api_def(
    PREST_API_DEF pApiDef
    )
{
    if(pApiDef)
    {
        SAFE_FREE_MEMORY(pApiDef->pszHost);
        SAFE_FREE_MEMORY(pApiDef->pszBasePath);
        coapi_free_api_module(pApiDef->pModules);
        SAFE_FREE_MEMORY(pApiDef);
    }
}
