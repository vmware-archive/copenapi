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
//main.c
uint32_t
rest_exec(
    PREST_API_DEF pApiDef,
    PPARSE_CONTEXT pContext
    );

//help.c
uint32_t
show_help(
    PCMD_ARGS pArgs,
    PREST_API_DEF pApiDef
    );

uint32_t
show_modules(
    PREST_API_DEF pApiDef
    );

uint32_t
show_module_commands(
    PREST_API_MODULE pModule
    );

uint32_t
show_method(
    PREST_API_MODULE pModule,
    const char *pszMethod
    );
//parseargs.c
uint32_t
parse_main_args(
    int argc,
    char **argv,
    PCMD_ARGS *ppCmdArgs
    );

uint32_t
collect_extra_args(
    int argIndex,
    int argc,
    char* const* argv,
    char*** pppszCmds,
    int* pnCmdCount
    );

uint32_t
parse_option(
    const char* pszName,
    const char* pszArg,
    PCMD_ARGS pCmdArgs
    );

void
free_cmd_args(
    PCMD_ARGS pCmdArgs
    );

//param.c
uint32_t
params_parse_string(
    const char *pszString,
    PPARSE_CONTEXT pContext
    );

uint32_t
params_parse(
    int argc,
    const char **argv,
    PPARSE_CONTEXT pContext
    );

uint32_t
param_extract(
    int argc,
    int argn,
    const char **argv,
    PPARAM_SPEC pSpec,
    PKEYVALUE *ppValue
    );

uint32_t
param_get_value(
    const char *pszArg,
    char **ppszValue
    );

uint32_t
param_get_flag(
    const char *pszFlag,
    char **ppszKey
    );

uint32_t
param_is_flag(
    const char *pszArg,
    int *pnFlag
    );

uint32_t
has_param(
    PPARSE_CONTEXT pContext,
    const char *pszName
    );

uint32_t
get_param_by_name(
    PPARSE_CONTEXT pContext,
    const char *pszName,
    PPARAM *ppParam
    );

void
print_context(
    PPARSE_CONTEXT pContext
    );

void
free_parse_context(
    PPARSE_CONTEXT pContext
    );

//parseargs_api.c
uint32_t
parse_api_args(
   int argc,
   char **argv,
   PREST_API_DEF pApiDef
   );

//restclient.c
uint32_t
call_rest_method(
    );

//utils.c
uint32_t
has_module(
    PREST_API_DEF pApiDef,
    const char *pszName,
    int *pnHasModule
    );

uint32_t
get_default_api_spec(
    char **ppszApiSpec
    );

void
show_error(
    uint32_t dwError
    );
