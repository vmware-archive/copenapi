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

struct slist_holder
{
    struct curl_slist **list;
};

int trace_fn(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp);

static uint32_t
_apply_single_header(void *userdata, const char *key, const char *value)
{
    uint32_t dwError = 0;
    struct slist_holder *holder = (struct slist_holder *)userdata;
    char *pszHeader = NULL;
    if (!holder|| !key || !value)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

//if (strcasecmp("authorization", key) == 0) goto error;

    dwError = coapi_allocate_string_printf(&pszHeader, "%s:%s", key, value);
    BAIL_ON_ERROR(dwError);

    *(holder->list) = curl_slist_append(*(holder->list), pszHeader);

error:
    SAFE_FREE_MEMORY(pszHeader);
    return dwError;
}

static uint32_t
_apply_header(PCMD_ARGS pArgs, CURL *pCurl, struct curl_slist **list) {
    int dwError = 0;
    struct slist_holder holder = {0};

    if (!pArgs || !pArgs->pConfigData || !pCurl || !list)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    holder.list = list;
    dwError = get_default_headers(pArgs->pConfigData,
                                  &holder,
                                  _apply_single_header
                                 );
    BAIL_ON_ERROR(dwError);

    if (*list)
    {
        curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, *list);
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

uint32_t
call_rest_method(
    const char *pszUrl,
    PREST_API_METHOD pMethod,
    PCMD_ARGS pArgs
    )
{
    uint32_t dwError = 0;
    CURL *pCurl = NULL;
    CURLcode res = CURLE_OK;
    long nStatus = 0;
    struct curl_slist *list = NULL;

    if(IsNullOrEmptyString(pszUrl) || !pMethod || !pArgs)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pCurl = curl_easy_init();
    if(pCurl)
    {
        //if netrc is specified, dont look for cmd line user/pass
        if(pArgs->nNetrc)
        {
            dwError = curl_easy_setopt(pCurl, CURLOPT_NETRC, CURL_NETRC_REQUIRED);
            BAIL_ON_CURL_ERROR(dwError);
        }
        else if(!IsNullOrEmptyString(pArgs->pszUserPass))
        {
            dwError = curl_easy_setopt(pCurl, CURLOPT_USERPWD, pArgs->pszUserPass);
            BAIL_ON_CURL_ERROR(dwError);
        }
        if(pArgs->nInsecure)
        {
            dwError = curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, 0L);
            BAIL_ON_CURL_ERROR(dwError);
            dwError = curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
            BAIL_ON_CURL_ERROR(dwError);
        }
        if(pArgs->nVerbose)
        {
            fprintf(stdout, "URL: %s\n", pszUrl);

            dwError = curl_easy_setopt(pCurl, CURLOPT_DEBUGFUNCTION, trace_fn);
            BAIL_ON_CURL_ERROR(dwError);

            dwError = curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 1L);
            BAIL_ON_CURL_ERROR(dwError);
        }
        dwError = curl_easy_setopt(pCurl, CURLOPT_FOLLOWLOCATION, 1L);
        BAIL_ON_CURL_ERROR(dwError);

        dwError = curl_easy_setopt(pCurl, CURLOPT_URL, pszUrl);
        BAIL_ON_CURL_ERROR(dwError);

        switch(pMethod->nMethod)
        {
            case METHOD_PUT:
                dwError = curl_easy_setopt(pCurl, CURLOPT_CUSTOMREQUEST, "PUT");
                dwError = curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, "");
                break;
            case METHOD_POST:
                dwError = curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, "");
                break;
            case METHOD_DELETE:
                dwError = curl_easy_setopt(pCurl, CURLOPT_CUSTOMREQUEST, "DELETE");
                break;
            case METHOD_PATCH:
                dwError = curl_easy_setopt(pCurl, CURLOPT_CUSTOMREQUEST, "PATCH");
                break;
            default:
                break;
            BAIL_ON_CURL_ERROR(dwError);
        }

        dwError = _apply_header(pArgs, pCurl, &list);
        BAIL_ON_CURL_ERROR(dwError);

        dwError = curl_easy_perform(pCurl);
        BAIL_ON_CURL_ERROR(dwError);

        fprintf(stdout, "\n");

        dwError = curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, &nStatus);
        BAIL_ON_CURL_ERROR(dwError);

        if(nStatus != HTTP_OK)
        {
            if(nStatus == 401)
            {
                fprintf(stderr,
                        "Error: 401. Unauthorized. Specify user with --user or -u\n");
            }
            else
            {
                fprintf(stderr, "Error: server returned %ld\n", nStatus);
            }
            dwError = 1;
            BAIL_ON_ERROR(dwError);
        }
    }

cleanup:
    if (list)
    {
        curl_slist_free_all(list);
    }
    if(pCurl)
    {
        curl_easy_cleanup(pCurl);
    }
    return dwError;

error:
    show_error(dwError);
    goto cleanup;
}


uint32_t
get_method_spec(
    PREST_API_MODULE pModules,
    const char *pszModule,
    const char *pszCmd,
    PREST_API_ENDPOINT *ppEndpoint
    )
{
    uint32_t dwError = 0;
    PREST_API_MODULE pModule = NULL;
    PREST_API_ENDPOINT pEndpoints = NULL;
    PREST_API_ENDPOINT pEndpoint = NULL;
    int nPossibleMatches = 0;
    int nExactMatches = 0;

    if(!pModules ||
       IsNullOrEmptyString(pszModule) ||
       IsNullOrEmptyString(pszCmd) ||
       !ppEndpoint)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_find_module_by_name(
                  pszModule,
                  pModules,
                  &pModule);
    BAIL_ON_ERROR(dwError);

    pEndpoints = pModule->pEndPoints;
    while(pEndpoints)
    {
        char *pszFind = strstr(pEndpoints->pszActualName, pszCmd);
        //must match at the end
        if(pszFind && !strcasecmp(pszFind, pszCmd))
        {
            //is this a full match? then we can stop here
            if(!strcasecmp(pEndpoints->pszName, pszCmd))
            {
                pEndpoint = pEndpoints;
                break;
            }
            else if(pEndpoints->pszName < pszFind && *(pszFind - 1) == '/')
            {
                pEndpoint = NULL;
                if(!nExactMatches)
                {
                    pEndpoint = pEndpoints;
                }
                ++nExactMatches;
            }
            else if(!nExactMatches)
            {
                if(!nPossibleMatches)
                {
                    pEndpoint = pEndpoints;
                }
                else
                {
                    pEndpoint = NULL;
                }
                ++nPossibleMatches;
            }
        }
        pEndpoints = pEndpoints->pNext;
    }

    if(!pEndpoint)
    {
        if(nExactMatches)
        {
            fprintf(
                stdout,
                "%d commands exactly match the search.\n"
                "Please specify command. For eg. to resolve ambiguity between\n"
                "ip/addr and ipv6/addr, you can specify ip/addr as the command.\n",
                nExactMatches);
            //show exact matches
            pEndpoints = pModule->pEndPoints;
            while(pEndpoints)
            {
                char *pszFind = strstr(pEndpoints->pszActualName, pszCmd);
                if(pszFind && !strcasecmp(pszFind, pszCmd))
                {
                    if(pEndpoints->pszActualName < pszFind && *(pszFind - 1) == '/')
                    {
                        fprintf(stdout, "%s\n", pEndpoints->pszActualName);
                    }
                }
                pEndpoints = pEndpoints->pNext;
            }
        }
        else if(nPossibleMatches)
        {
            fprintf(
                stdout,
                "%d commands partially match the search.\n"
                "Please specify command.\n",
                nPossibleMatches);
            //show potential matches
            pEndpoints = pModule->pEndPoints;
            while(pEndpoints)
            {
                char *pszFind = strstr(pEndpoints->pszActualName, pszCmd);
                if(pszFind && !strcasecmp(pszFind, pszCmd))
                {
                    fprintf(stdout, "%s\n", pEndpoints->pszActualName);
                }
                pEndpoints = pEndpoints->pNext;
            }
        }
        dwError = ENOENT;
        if(nExactMatches || nPossibleMatches)
        {
            dwError = ENOTUNIQ;
        }
        BAIL_ON_ERROR(dwError);
    }

    *ppEndpoint = pEndpoint;

cleanup:
    return dwError;

error:
    if(ppEndpoint)
    {
        *ppEndpoint = NULL;
    }
    goto cleanup;
}

uint32_t
replace_path(
    const char *pszEndpointIn,
    const char *pszName,
    const char *pszValue,
    char **ppszEndpoint
    )
{
    uint32_t dwError = 0;
    char *pszEndpoint = NULL;
    char *pszPath = NULL;

    dwError = coapi_allocate_string_printf(&pszPath, "{%s}", pszName);
    BAIL_ON_ERROR(dwError);

    dwError = string_replace(
                  pszEndpointIn,
                  pszPath,
                  pszValue,
                  &pszEndpoint);
    BAIL_ON_ERROR(dwError);

    *ppszEndpoint = pszEndpoint;
cleanup:
    SAFE_FREE_MEMORY(pszPath);
    return dwError;

error:
    if(ppszEndpoint)
    {
        *ppszEndpoint = NULL;
    }
    SAFE_FREE_MEMORY(pszEndpoint);
    goto cleanup;
}

uint32_t
get_query_string(
    PREST_CMD_ARGS pRestArgs,
    PREST_API_PARAM pApiParams,
    char **ppszQuery
    )
{
    uint32_t dwError = 0;
    int i = 0;
    PREST_CMD_PARAM pParam = NULL;
    PREST_API_PARAM pApiParam = NULL;
    char *pszQuery = NULL;
    char *pszTemp = NULL;
    char *pszKeyValue = NULL;

    if(!pRestArgs || !ppszQuery)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    for(pApiParam = pApiParams; pApiParam; pApiParam = pApiParam->pNext)
    {
        const char *pszName = pApiParam->pszName;

        dwError = get_param_by_name(pRestArgs, pszName, &pParam);
        if(dwError == ENOENT)
        {
            if(!pApiParam->nRequired)
            {
                continue;
            }
            fprintf(stderr, "please provide required param --%s\n", pszName);
        }
        BAIL_ON_ERROR(dwError);

        if(IsNullOrEmptyString(pParam->pszValue))
        {
            if(pParam->nRequired)
            {
                fprintf(stderr, "value required for param --%s\n", pszName);
                dwError = EINVAL;
                BAIL_ON_ERROR(dwError);
            }
            continue;
        }

	/* only process query params */
        if (strcmp(pApiParam->pszIn, "query"))
        {
            continue;
        }

        dwError = coapi_allocate_string_printf(
                      &pszKeyValue,
                      "%s=%s",
                      pParam->pszName,
                      pParam->pszValue);
        BAIL_ON_ERROR(dwError);

        pszTemp = pszQuery;
        pszQuery = NULL;
        dwError = coapi_allocate_string_printf(
                      &pszQuery,
                      "%s%s%s",
                      pszTemp ? pszTemp : "",
                      pszTemp ? "&" : "",
                      pszKeyValue);
        BAIL_ON_ERROR(dwError);

        SAFE_FREE_MEMORY(pszKeyValue);
        SAFE_FREE_MEMORY(pszTemp);
        pszKeyValue = NULL;
        pszTemp = NULL;
    }

    *ppszQuery = pszQuery;
cleanup:
    SAFE_FREE_MEMORY(pszKeyValue);
    SAFE_FREE_MEMORY(pszTemp);
    return dwError;

error:
    if(ppszQuery)
    {
        *ppszQuery = NULL;
    }
    SAFE_FREE_MEMORY(pszQuery);
    goto cleanup;
}

uint32_t
get_method_spec_by_operation_id(
    PREST_API_MODULE pModules,
    const char *pszModule,
    const char *pszCmd,
    PREST_API_ENDPOINT *ppEndpoint,
    PREST_API_METHOD *ppMethod
    )
{
    uint32_t dwError = 0;
    PREST_API_MODULE pModule = NULL;
    PREST_API_ENDPOINT pEndpoints = NULL;
    PREST_API_ENDPOINT pEndpoint = NULL;
    PREST_API_METHOD pMethod = NULL;
    PREST_API_METHOD pMethodTemp = NULL;
    RESTMETHOD method = METHOD_GET;

    if(!pModules ||
       IsNullOrEmptyString(pszModule) ||
       IsNullOrEmptyString(pszCmd) ||
       !ppMethod)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_find_module_by_name(
                  pszModule,
                  pModules,
                  &pModule);
    BAIL_ON_ERROR(dwError);

    pEndpoints = pModule->pEndPoints;
    for(; pEndpoints; pEndpoints = pEndpoints->pNext)
    {
	/* need only consider path substitutes at end for this */
	if (pEndpoints->pszCommandName[0] != '{')
            continue;

	for(method = METHOD_GET; method < METHOD_COUNT; method++)
	{
	    pMethodTemp = pEndpoints->pMethods[method];
	    if (pMethodTemp && pMethodTemp->pszOperationId)
	    {
                if(strcmp(pszCmd, pMethodTemp->pszOperationId) == 0)
		{
		    pEndpoint = pEndpoints;
		    pMethod = pMethodTemp;
		    break;
		}
	    }
	}
    }

    if (!pMethod || !pEndpoint)
    {
        dwError = ENOENT;
	BAIL_ON_ERROR(dwError);
    }
    *ppEndpoint = pEndpoint;
    *ppMethod = pMethod;

cleanup:
    return dwError;

error:
    if(ppMethod)
    {
        *ppMethod = NULL;
    }
    if(ppEndpoint)
    {
        *ppEndpoint = NULL;
    }
    goto cleanup;
}

uint32_t
rest_get_method(
    PREST_API_DEF pApiDef,
    PREST_CMD_ARGS pRestArgs,
    PREST_API_ENDPOINT *ppEndpoint,
    PREST_API_METHOD *ppMethod
    )
{
    uint32_t dwError = 0;
    PREST_API_METHOD pMethod = NULL;
    PREST_API_ENDPOINT pEndpoint = NULL;
    RESTMETHOD nRestMethod = METHOD_GET;
    char *pszMethods[] = {"GET", "PUT", "POST", "DELETE", "PATCH"};

    if(!pApiDef || !pRestArgs || !ppMethod)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    nRestMethod = pRestArgs->nRestMethod;

    dwError = get_method_spec(
                  pApiDef->pModules,
                  pRestArgs->pszModule,
                  pRestArgs->pszCmd,
                  &pEndpoint
                  );
    if(dwError == ENOENT)
    {
	/* check if there is an operationId with this name
	 * operationId is supposed to be unique so we expect a rest method
	 * in return as well
	 */
	dwError = get_method_spec_by_operation_id(
			pApiDef->pModules,
			pRestArgs->pszModule,
			pRestArgs->pszCmd,
			&pEndpoint,
			&pMethod);
    }
    if (dwError == ENOENT)
    {
        fprintf(stderr,
                "There is no command named %s under module %s\n",
                pRestArgs->pszCmd,
                pRestArgs->pszModule);
    }
    BAIL_ON_ERROR(dwError);

    if (pMethod)
    {
        goto foundMethod;
    }

    if(nRestMethod >= METHOD_GET && nRestMethod <= METHOD_PATCH)
    {
        pMethod = pEndpoint->pMethods[nRestMethod];
        if(!pMethod)
        {
            fprintf(stderr,
                    "%s method not found for command: %s\n",
                    pszMethods[nRestMethod],
                    pRestArgs->pszCmd);
        }
    }
    else if(pEndpoint->pMethods[METHOD_GET])
    {
        pRestArgs->nRestMethod = METHOD_GET;
        pMethod = pEndpoint->pMethods[METHOD_GET];
    }
    else
    {
        int i = 0;
        int nCount = 0;
        for(i = METHOD_GET; i < METHOD_COUNT; ++i)
        {
            if(pEndpoint->pMethods[i])
            {
                nRestMethod = i;
                ++nCount;
            }
        }
        if(nCount > 1)
        {
            fprintf(stdout,
                    "Multiple methods found. Please specify with -X\n");

            for(i = METHOD_GET; i < METHOD_COUNT; ++i)
            {
                if(pEndpoint->pMethods[i])
                {
                    fprintf(stdout, "-X %s\n", pszMethods[i]);
                }
            }
        }
        else if(nCount == 1)
        {
            pRestArgs->nRestMethod = nRestMethod;
            pMethod = pEndpoint->pMethods[nRestMethod];
        }
    }

    if(!pMethod)
    {
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }

foundMethod:
    *ppMethod = pMethod;
    if(ppEndpoint)
    {
        *ppEndpoint = pEndpoint;
    }
cleanup:
    return dwError;

error:
    if(ppMethod)
    {
        *ppMethod = NULL;
    }
    if(ppEndpoint)
    {
        *ppEndpoint = NULL;
    }
    goto cleanup;
}

uint32_t
rest_get_cmd_params(
    PREST_API_DEF pApiDef,
    PREST_CMD_ARGS pRestArgs
    )
{
    uint32_t dwError = 0;
    PREST_API_PARAM pApiParam = NULL;
    PREST_API_METHOD pMethod = NULL;
    PREST_CMD_PARAM pParam = NULL;
    int nParamCount = 0;

    if(!pApiDef || !pRestArgs)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = rest_get_method(pApiDef, pRestArgs, NULL, &pMethod);
    BAIL_ON_ERROR(dwError);

    for(pApiParam = pMethod->pParams, nParamCount = 0;
        pApiParam;
        pApiParam = pApiParam->pNext, ++nParamCount)
    {
        const char *pszName = pApiParam->pszName;

        dwError = coapi_allocate_memory(
                      sizeof(REST_CMD_PARAM),
                      (void **)&pParam);
        BAIL_ON_ERROR(dwError);

        pParam->nRequired = pApiParam->nRequired;
        dwError = coapi_allocate_string(pApiParam->pszName, &pParam->pszName);
        BAIL_ON_ERROR(dwError);

        pParam->pNext = pRestArgs->pParams;
        pRestArgs->pParams = pParam;
        pParam = NULL;
    }
    pRestArgs->nParamCount = nParamCount;
cleanup:
    return dwError;

error:
    free_rest_cmd_params(pParam);
    goto cleanup;
}

static uint32_t
get_url_string(
    const char *pszPathIn,
    PREST_CMD_ARGS pRestArgs,
    PREST_API_PARAM pApiParam,
    char **ppszPathOut
    )
{
    uint32_t dwError = 0;
    char *pszPath = NULL;
    char *pszPathTemp = NULL;
    PREST_CMD_PARAM pParam = NULL;

    for(; pApiParam; pApiParam = pApiParam->pNext)
    {
        if(strcmp(pApiParam->pszIn, "path"))
        {
            continue;
        }

        dwError = get_param_by_name(pRestArgs, pApiParam->pszName, &pParam);
        BAIL_ON_ERROR(dwError);

        dwError = replace_path(pszPathIn, pParam->pszName, pParam->pszValue, &pszPathTemp);
        BAIL_ON_ERROR(dwError);

        if(dwError == ENOENT)
        {
            fprintf(stderr,
                    "path '%s' specified with bad path string: %s\n",
                    pApiParam->pszName,
                    pszPathIn);
        }
        BAIL_ON_ERROR(dwError);

        SAFE_FREE_MEMORY(pszPath);
        pszPath = pszPathTemp;
        pszPathTemp = NULL;
    }

    if(!pszPath)
    {
        dwError = coapi_allocate_string(pszPathIn, &pszPath);
        BAIL_ON_ERROR(dwError);
    }

    *ppszPathOut = pszPath;

cleanup:
    SAFE_FREE_MEMORY(pszPathTemp);
    return dwError;

error:
    goto cleanup;
}

uint32_t
rest_exec(
    PREST_API_DEF pApiDef,
    PCMD_ARGS pArgs,
    PREST_CMD_ARGS pRestArgs
    )
{
    uint32_t dwError = 0;
    PREST_API_METHOD pMethod = NULL;
    char *pszUrl = NULL;
    char *pszEndpoint = NULL;
    PREST_API_ENDPOINT pEndpoint = NULL;
    char *pszParams = NULL;
    char *pszPath = NULL;

    if(!pApiDef || !pArgs || !pRestArgs)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = rest_get_method(pApiDef, pRestArgs, &pEndpoint, &pMethod);
    BAIL_ON_ERROR(dwError);

    dwError = get_query_string(pRestArgs, pMethod->pParams, &pszParams);
    if(dwError == ENOENT)
    {
        dwError = 0;
    }
    BAIL_ON_ERROR(dwError);

    dwError = get_url_string(pEndpoint->pszActualName, pRestArgs, pMethod->pParams, &pszPath);
    BAIL_ON_ERROR(dwError);

    if(IsNullOrEmptyString(pArgs->pszBaseUrl))
    {
        dwError = coapi_allocate_string_printf(
                      &pszUrl,
                      "%s://%s%s%s%s",
                      pApiDef->nHasSecureScheme ? "https" : "http",
                      pApiDef->pszHost,
                      pszPath,
                      pszParams ? "?" : "",
                      pszParams ? pszParams : "");
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        dwError = coapi_allocate_string_printf(
                      &pszUrl,
                      "%s%s%s%s",
                      pArgs->pszBaseUrl,
                      pszPath,
                      pszParams ? "?" : "",
                      pszParams ? pszParams : "");
        BAIL_ON_ERROR(dwError);
    }

    dwError = call_rest_method(pszUrl, pMethod, pArgs);
    BAIL_ON_ERROR(dwError);

cleanup:
    SAFE_FREE_MEMORY(pszPath);
    SAFE_FREE_MEMORY(pszParams);
    SAFE_FREE_MEMORY(pszUrl);
    SAFE_FREE_MEMORY(pszEndpoint);
    return dwError;

error:
    goto cleanup;
}

int trace_fn(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
{
    const char *text = NULL;
    switch (type)
    {
        case CURLINFO_HEADER_OUT:
            text = "=> Send header";
            break;
        case CURLINFO_DATA_OUT:
            text = "=> Send data";
            break;
        case CURLINFO_SSL_DATA_OUT:
            text = "=> Send SSL data";
            break;
        case CURLINFO_HEADER_IN:
            text = "<= Recv header";
            break;
      case CURLINFO_DATA_IN:
          text = "<= Recv data";
          break;
      case CURLINFO_SSL_DATA_IN:
          text = "<= Recv SSL data";
          break;
      default:
          return 0;
    }
    if(text)
    {
        size_t i = 0;
        fprintf(stderr, text);
        fprintf(stderr, "\n");
        for(i = 0; i < size; ++i)
        {
            fprintf(stderr, "%c", data[i]);
        }
        fprintf(stderr, "\n");
    }
    return 0;
}
