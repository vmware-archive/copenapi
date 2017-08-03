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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PARAM_SPEC_
{
    char *pszName;
    char chFlag;
    int nArgRequired;
}PARAM_SPEC, *PPARAM_SPEC;

typedef struct _PARAM_
{
    char *pszName;
    char *pszValue;
    struct _PARAM_ *pNext;
}PARAM, *PPARAM;

typedef enum _PARSE_STATE_
{
    PARSE_STATE_BEGIN,
    PARSE_STATE_READY,
    PARSE_STATE_FLAG,
    PARSE_STATE_FLAGVALUE
}PARSE_STATE;

typedef struct _PARSE_CONTEXT_
{
    PARSE_STATE parseState;
    char *pszModule;
    char *pszCmd;
    PPARAM pParams;
}PARSE_CONTEXT, *PPARSE_CONTEXT;

typedef struct _REST_CMD_PARAM_
{
    int nRequired;
    char *pszName;
    char *pszValue;
    struct _REST_CMD_PARAM_ *pNext;
}REST_CMD_PARAM, *PREST_CMD_PARAM;

typedef struct _REST_CMD_ARGS_
{
    int nParamCount;
    RESTMETHOD nRestMethod;
    char *pszModule;
    char *pszCmd;
    PREST_CMD_PARAM pParams;
}REST_CMD_ARGS, *PREST_CMD_ARGS;

typedef struct _CMD_ARGS_
{
    char *pszApiSpec;
    char *pszBaseUrl;
    char *pszUser;
    char *pszDomain;
    char *pszUserPass;
    char *pszSpn;
    int nCmdCount;
    int nHelp;
    int nVerbose;
    int nInsecure;
    int nNetrc;
    int nCmdIndex;
    RESTMETHOD nRestMethod;
    char **ppszCmds;
}CMD_ARGS, *PCMD_ARGS;

#ifdef __cplusplus
}
#endif
