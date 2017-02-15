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

#pragma once

uint32_t
coapi_find_tagged_module(
    json_t *pPath,
    PREST_API_MODULE pModules,
    PREST_API_MODULE *ppModule
    );

//jsonutils.c
uint32_t
get_json_object_from_string(
    const char *pszString,
    json_t **ppJsonObject
    );

uint32_t
json_safe_get_string_value(
    json_t *pRoot,
    const char *pszKey,
    char **ppszValue
    );

uint32_t
json_get_string_value(
    json_t *pRoot,
    const char *pszKey,
    char **ppszValue
    );

//utils.c
uint32_t
coapi_file_read_all_text(
    const char *pszFileName,
    char **ppszText
    );

//restapidef.c
uint32_t
coapi_load_modules(
    json_t *pRoot,
    PREST_API_MODULE *ppApiModules
    );

uint32_t
coapi_load_endpoints(
    json_t *pRoot,
    const char *pszBasePath,
    PREST_API_MODULE pApiModules
    );

uint32_t
coapi_load_parameters(
    json_t *pMethod,
    PREST_API_PARAM *ppParam
    );

uint32_t
coapi_load_secure_scheme(
    json_t *pRoot,
    int *pnHasSecure
    );

uint32_t
coapi_module_add_endpoint(
    PREST_API_MODULE pModule,
    PREST_API_ENDPOINT pEndPoint
    );

uint32_t
coapi_add_default_module(
    const char *pszModuleName,
    PREST_API_MODULE *ppApiModules
    );

void
coapi_free_api_param(
    PREST_API_PARAM pParam
    );

void
coapi_free_api_endpoint(
    PREST_API_ENDPOINT pEndPoint
    );

void
coapi_free_api_method(
    PREST_API_METHOD pMethod
    );

void
coapi_free_api_module(
    PREST_API_MODULE pModule
    );
