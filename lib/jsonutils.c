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
show_json_error(
    json_error_t *pError
    )
{
    if(!pError)
    {
        return;
    }
    fprintf(stderr, "error reading apispec: \n line: %d\n error: %s\n",
            pError->line,
            pError->text);
}

uint32_t
get_json_object_from_string(
    const char *pszString,
    json_t **ppJsonObject
    )
{
    uint32_t dwError = 0;
    json_t *pObject = NULL;
    json_error_t stError = {0};

    if(IsNullOrEmptyString(pszString) || !ppJsonObject)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pObject = json_loads(pszString, 0, &stError);
    if(!pObject)
    {
        dwError = EINVAL;
    }
    BAIL_ON_ERROR(dwError);

    *ppJsonObject = pObject;
cleanup:
    return dwError;

error:
    show_json_error(&stError);

    goto cleanup;
}

uint32_t
json_safe_get_string_value(
    json_t *pRoot,
    const char *pszKey,
    char **ppszValue
    )
{
    uint32_t dwError = 0;
    dwError = json_get_string_value(pRoot, pszKey, ppszValue);
    if(dwError == ENOENT)
    {
        dwError = 0;
    }
    return dwError;
}


uint32_t
json_get_string_value(
    json_t *pRoot,
    const char *pszKey,
    char **ppszValue
    )
{
    uint32_t dwError = 0;
    json_t *pJson = NULL;
    char *pszValue = NULL;

    if(!pRoot || !pszKey || !ppszValue)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pJson = json_object_get(pRoot, pszKey);
    if(!pJson)
    {
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_allocate_string(json_string_value(pJson), &pszValue);
    BAIL_ON_ERROR(dwError);

    *ppszValue = pszValue;

cleanup:
    return dwError;

error:
    if(ppszValue)
    {
        *ppszValue = NULL;
    }
    SAFE_FREE_MEMORY(pszValue);
    goto cleanup;
}
