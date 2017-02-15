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
coapi_load_from_string(
    const char *pszString,
    PREST_API_DEF *ppApiDef
    )
{
    uint32_t dwError = 0;
    json_t *pRoot = NULL;
    PREST_API_DEF pApiDef = NULL;

    if(!pszString)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }
    
    dwError = get_json_object_from_string(pszString, &pRoot);
    BAIL_ON_ERROR(dwError);

    dwError = coapi_allocate_memory(sizeof(REST_API_DEF), (void **)&pApiDef);
    BAIL_ON_ERROR(dwError);

    dwError = coapi_load_secure_scheme(pRoot, &pApiDef->nHasSecureScheme);
    //default to https if not specified
    if(dwError == ENODATA)
    {
        dwError = 0;
        pApiDef->nHasSecureScheme = 1;
    }
    BAIL_ON_ERROR(dwError);

    dwError = json_get_string_value(pRoot, "host", &pApiDef->pszHost);
    BAIL_ON_ERROR(dwError);

    dwError = json_get_string_value(pRoot, "basePath", &pApiDef->pszBasePath);
    BAIL_ON_ERROR(dwError);

    dwError = coapi_load_modules(pRoot, &pApiDef->pModules);
    if(dwError == ENODATA)
    {
        dwError = coapi_add_default_module(
                      pApiDef->pszBasePath,
                      &pApiDef->pModules);
        BAIL_ON_ERROR(dwError);

        pApiDef->nNoModules = 1;
    }

    BAIL_ON_ERROR(dwError);

    dwError = coapi_load_endpoints(pRoot, pApiDef->pszBasePath, pApiDef->pModules);
    BAIL_ON_ERROR(dwError);

    *ppApiDef = pApiDef;
cleanup:
    if(pRoot)
    {
        json_decref(pRoot);
    }
    return dwError;

error:
    SAFE_FREE_MEMORY(pApiDef);
    goto cleanup;
}

uint32_t
coapi_load_from_file(
    const char *pszFile,
    PREST_API_DEF *ppApiDef
    )
{
    uint32_t dwError = 0;
    char *pszJson = NULL;
    PREST_API_DEF pApiDef = NULL;

    if(!pszFile || !ppApiDef)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_file_read_all_text(pszFile, &pszJson);
    BAIL_ON_ERROR(dwError);

    dwError = coapi_load_from_string(pszJson, &pApiDef);
    BAIL_ON_ERROR(dwError);

    *ppApiDef = pApiDef;

cleanup:
    SAFE_FREE_MEMORY(pszJson);
    return dwError;

error:
    SAFE_FREE_MEMORY(pApiDef);
    goto cleanup;
}
