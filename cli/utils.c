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
has_module(
    PREST_API_DEF pApiDef,
    const char *pszModule,
    int *pnHasModule
    )
{
    uint32_t dwError = 0;
    PREST_API_MODULE pModule = NULL;
    int nHasModule = 0;

    if(!pApiDef || IsNullOrEmptyString(pszModule) || !pnHasModule)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    for(pModule = pApiDef->pModules; pModule; pModule = pModule->pNext)
    {
        if(!strcasecmp(pModule->pszName, pszModule))
        {
            nHasModule = 1;
            break;
        }
    }

    *pnHasModule = nHasModule;

cleanup:
    return dwError;

error:
    if(pnHasModule)
    {
        *pnHasModule = 0;
    }
    goto cleanup;
}

uint32_t
get_default_int(
    PCONF_DATA pData,
    const char *pszKey,
    int *pnValue
    )
{
   uint32_t dwError = 0;
   char *szValue = NULL;

   dwError = get_default_value(pData, pszKey, &szValue);
   BAIL_ON_ERROR(dwError);

   *pnValue = atoi(szValue);

error:
   SAFE_FREE_MEMORY(szValue);
   return dwError;
}

uint32_t
get_default_value(
    PCONF_DATA pData,
    const char *pszKey,
    char **ppszValue
    )
{
    uint32_t dwError = 0;
    char *pszValue = NULL;
    PCONF_SECTION pSection = NULL;
    PKEYVALUE pKeyValues = NULL;

    if(!pData || !pszKey || !ppszValue)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = config_get_section(pData, "default", &pSection);
    BAIL_ON_ERROR(dwError);

    pKeyValues = pSection->pKeyValues;
    for(; pKeyValues; pKeyValues = pKeyValues->pNext)
    {
        if(!strcmp(pKeyValues->pszKey, pszKey))
        {
            dwError = coapi_allocate_string(
                          pKeyValues->pszValue,
                          &pszValue);
            BAIL_ON_ERROR(dwError);
        }
    }

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

uint32_t
get_home_dir(
    char **ppszDir
    )
{
    uint32_t dwError = 0;
    char *pszDir = NULL;
    const char *pszHomeDir = NULL;

    if(!ppszDir)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pszHomeDir = getenv("HOME");
    if(!pszHomeDir)
    {
        pszHomeDir = getpwuid(getuid())->pw_dir;
    }

    if(IsNullOrEmptyString(pszHomeDir))
    {
        fprintf(stderr, "invalid homedir %s\n", pszHomeDir);
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_allocate_string(pszHomeDir, &pszDir);
    BAIL_ON_ERROR(dwError);

    *ppszDir = pszDir;

cleanup:
    return dwError;

error:
    if(ppszDir)
    {
        *ppszDir = NULL;
    }
    SAFE_FREE_MEMORY(pszDir);
    goto cleanup;
}

static uint32_t
_get_default_config_name(
    char **ppszConfig
    )
{
    uint32_t dwError = 0;
    char pszCWD[MAXPATHLEN] = {0};
    char *pszConfig = NULL;
    char *pszHomeDir = NULL;

    if(!ppszConfig)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    if(!getcwd(pszCWD, MAXPATHLEN))
    {
        dwError = errno;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_allocate_string_printf(
                  &pszConfig,
                  "%s/%s",
                  pszCWD,
                  COPENAPI_CONFIG_FILE);
    BAIL_ON_ERROR(dwError);

    if(access(pszConfig, F_OK))
    {
        if(errno != ENOENT)
        {
            dwError = errno;
            BAIL_ON_ERROR(dwError);
        }
        SAFE_FREE_MEMORY(pszConfig);
        pszConfig = NULL;
    }

    if(!pszConfig)
    {
        dwError = get_home_dir(&pszHomeDir);
        BAIL_ON_ERROR(dwError);

        dwError = coapi_allocate_string_printf(
                      &pszConfig,
                      "%s/%s",
                      pszHomeDir,
                      COPENAPI_CONFIG_FILE);
        BAIL_ON_ERROR(dwError);

        if(access(pszConfig, F_OK))
        {
            dwError = errno;
            BAIL_ON_ERROR(dwError);
        }
    }

    *ppszConfig = pszConfig;

cleanup:
    SAFE_FREE_MEMORY(pszHomeDir);
    return dwError;

error:
    if(ppszConfig)
    {
        *ppszConfig = NULL;
    }
    SAFE_FREE_MEMORY(pszConfig);
    goto cleanup;
}

uint32_t
get_config_data(PCONF_DATA *ppConfigData)
{
    uint32_t dwError = 0;
    char *pszConfig = NULL;
    PCONF_DATA pConfigData = NULL;

    if (!ppConfigData)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = _get_default_config_name(&pszConfig);
    BAIL_ON_ERROR(dwError);

    dwError = read_config_file(pszConfig, 0, &pConfigData);
    BAIL_ON_ERROR(dwError);

    *ppConfigData = pConfigData;

cleanup:
    SAFE_FREE_MEMORY(pszConfig);
    return dwError;

error:
    free_config_data(pConfigData);
    goto cleanup;
} 

uint32_t
get_default_headers(PCONF_DATA pData, void *userdata, PFN_HEADER_CB fn)
{
    uint32_t dwError = 0;
    PCONF_SECTION pSection = NULL;
    PKEYVALUE pKeyValues = NULL;
    size_t nHeader = 0;

    if(!pData || !fn)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    nHeader = strlen(CONFIG_KEY_HEADER);

    dwError = config_get_section(pData, "default", &pSection);
    BAIL_ON_ERROR(dwError);

    pKeyValues = pSection->pKeyValues;
    for(; pKeyValues; pKeyValues = pKeyValues->pNext)
    {
        if(!strncmp(pKeyValues->pszKey, CONFIG_KEY_HEADER, nHeader))
        {
            dwError = fn(userdata,
                         pKeyValues->pszKey+nHeader,
                         pKeyValues->pszValue);
            BAIL_ON_ERROR(dwError);
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

void
show_error(
    uint32_t dwError
    )
{
    if(dwError == 0)
    {
        return;
    }

    if(dwError >= ERROR_COPENAPI_CLI_CURL_BASE &&
       dwError < ERROR_COPENAPI_CLI_CURL_END)
    {
        CURLcode dwCurlError = dwError - ERROR_COPENAPI_CLI_CURL_BASE;
        fprintf(stderr, "error: %s\n", curl_easy_strerror(dwCurlError));
    }
}
