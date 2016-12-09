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

#pragma once

#include "copenapitypes.h"

uint32_t
coapi_load_from_string(
    const char *pszString,
    PREST_API_DEF *ppApiDef
    );

uint32_t
coapi_load_from_file(
    const char *pszFile,
    PREST_API_DEF *ppApiDef
    );

uint32_t
coapi_find_tagged_module(
    json_t *pPath,
    PREST_API_MODULE pModules,
    PREST_API_MODULE *ppModule
    );

uint32_t
coapi_find_module_by_name(
    const char *pszName,
    PREST_API_MODULE pModules,
    PREST_API_MODULE *ppModule
    );

uint32_t
coapi_find_endpoint_by_name(
    const char *pszName,
    PREST_API_ENDPOINT pEndPoints,
    PREST_API_ENDPOINT *ppEndPoint
    );

uint32_t
coapi_find_module_impl_by_name(
    const char *pszName,
    PREST_MODULE pModules,
    PREST_MODULE *ppModule
    );

uint32_t
coapi_is_integer(
    const char *pszValue,
    int *pnValid
    );

uint32_t
coapi_check_param(
    PREST_API_PARAM pParam,
    const char *pszValue,
    int *pnValid
    );

uint32_t
coapi_get_required_params(
    PREST_API_METHOD pMethod,
    PREST_API_PARAM **pppRequiredParams,
    int *pnRequiredParamsCount
    );

uint32_t
coapi_find_method(
    PREST_API_DEF pApiDef,
    const char *pszEndPoint,
    const char *pszMethod,
    PREST_API_METHOD *ppMethod
    );

uint32_t
coapi_find_handler(
    PREST_API_DEF pApiDef,
    const char *pszEndPoint,
    const char *pszMethod,
    PREST_API_METHOD *ppMethod
    );

uint32_t
coapi_get_rest_type(
    const char *pszType,
    RESTPARAMTYPE *pnType
    );

uint32_t
coapi_get_rest_method_string(
    RESTMETHOD nMethod,
    char **ppszMethod
    );

uint32_t
coapi_get_rest_method(
    const char *pszMethod,
    RESTMETHOD *pnMethod
    );

uint32_t
coapi_map_api_impl(
    PREST_API_DEF pApiDef,
    PMODULE_REG_MAP pRegMap
    );

uint32_t
coapi_map_module_impl(
    PREST_API_MODULE pModule,
    PREST_MODULE pModuleImpl
    );

void
coapi_print_api_def(
    PREST_API_DEF pApiDef
    );

void
coapi_free_api_def(
    PREST_API_DEF pApiDef
    );
