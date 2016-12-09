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

typedef enum _RESTMETHOD_
{
    METHOD_GET = 0,
    METHOD_PUT,
    METHOD_POST,
    METHOD_DELETE,
    METHOD_PATCH,
    METHOD_COUNT,
    METHOD_INVALID
}RESTMETHOD;

typedef enum _RESTPARAMTYPE_
{
    RESTPARAM_INTEGER = 0,
    RESTPARAM_STRING,
    RESTPARAM_ARRAY,
    RESTPARAM_INVALID
}RESTPARAMTYPE;

typedef uint32_t
(*PFN_MODULE_ENDPOINT_CB)(
     const char *pszJsonIn,
     char **pszJsonOut
    );

typedef struct _REST_MODULE_ *PREST_MODULE;
typedef uint32_t
(*PFN_GET_MODULE_REGISTRATION)(
    PREST_MODULE *ppRestModule
    );

typedef struct _REST_MODULE_
{
    char *pszEndPoint;
    PFN_MODULE_ENDPOINT_CB pFnEndPointMethods[METHOD_COUNT];
    struct _REST_MODULE_ *pNext;
}REST_MODULE, *PREST_MODULE;

typedef struct _MODULE_REG_MAP_
{
    const char *pszName;
    PFN_GET_MODULE_REGISTRATION pFnModuleReg;
}MODULE_REG_MAP, *PMODULE_REG_MAP;

typedef struct _REST_API_PARAM_
{
    char *pszName;
    char *pszIn;
    int nRequired;
    RESTPARAMTYPE nType;

    struct _REST_API_PARAM_ *pNext;
}REST_API_PARAM, *PREST_API_PARAM;

typedef struct _REST_API_METHOD_
{
    RESTMETHOD nMethod;
    char *pszMethod;
    char *pszSummary;
    char *pszDescription;
    PREST_API_PARAM pParams;
    PFN_MODULE_ENDPOINT_CB pFnImpl;
}REST_API_METHOD, *PREST_API_METHOD;

typedef struct _REST_API_ENDPOINT_
{
    char *pszName;
    char *pszCommandName;
    PREST_API_METHOD pMethods[METHOD_COUNT];
    struct _REST_API_ENDPOINT_ *pNext;
}REST_API_ENDPOINT, *PREST_API_ENDPOINT;

typedef struct _REST_API_MODULE_
{
    char *pszName;
    char *pszDescription;
    PREST_API_ENDPOINT pEndPoints;
    struct _REST_API_MODULE_ *pNext;
}REST_API_MODULE, *PREST_API_MODULE;

typedef struct _REST_API_DEF_
{
    char *pszHost;
    char *pszBasePath;
    PREST_API_MODULE pModules;
}REST_API_DEF, *PREST_API_DEF;
