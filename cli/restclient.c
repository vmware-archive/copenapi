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

#include "includes.h"

int trace_fn(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp);

static size_t
write_mem_cb(
    void *contents,
    size_t size,
    size_t nmemb,
    void *userp)
{
    printf("%s\n", (char *)contents);
    return size * nmemb;
}

uint32_t
call_rest_method(
    const char *pszUrl,
    PREST_API_METHOD pMethod,
    int nVerbose
    )
{
    uint32_t dwError = 0;
    CURL *pCurl = NULL;
    CURLcode res = CURLE_OK;

    if(IsNullOrEmptyString(pszUrl) || !pMethod)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    pCurl = curl_easy_init();
    if(pCurl)
    {
        if(nVerbose)
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

        dwError = curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, write_mem_cb);
        BAIL_ON_CURL_ERROR(dwError);

        dwError = curl_easy_perform(pCurl);
        BAIL_ON_CURL_ERROR(dwError);
    }

cleanup:
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
    PREST_API_ENDPOINT *ppEndPoint
    )
{
    uint32_t dwError = 0;
    PREST_API_MODULE pModule = NULL;
    PREST_API_ENDPOINT pEndPoints = NULL;
    PREST_API_ENDPOINT pEndPoint = NULL;
    int nPossibleMatches = 0;

    if(!pModules ||
       IsNullOrEmptyString(pszModule) ||
       IsNullOrEmptyString(pszCmd) ||
       !ppEndPoint)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = coapi_find_module_by_name(
                  pszModule,
                  pModules,
                  &pModule);
    BAIL_ON_ERROR(dwError);

    pEndPoints = pModule->pEndPoints;
    while(pEndPoints)
    {
        char *pszFind = strstr(pEndPoints->pszName, pszCmd);
        //must match at the end
        if(pszFind && !strcasecmp(pszFind, pszCmd))
        {
            //is this a full match? then we can stop here
            if(!strcasecmp(pEndPoints->pszName, pszCmd))
            {
                pEndPoint = pEndPoints;
                break;
            }
            else
            {
                if(!nPossibleMatches)
                {
                    pEndPoint = pEndPoints;
                }
                else
                {
                    pEndPoint = NULL;
                }
                ++nPossibleMatches;
            }
        }
        pEndPoints = pEndPoints->pNext;
    }

    if(!pEndPoint)
    {
        if(nPossibleMatches)
        {
            fprintf(
                stdout,
                "%d commands match the search. Please specify command.\n",
                nPossibleMatches);
            //show potential matches
            pEndPoints = pModule->pEndPoints;
            while(pEndPoints)
            {
                char *pszFind = strstr(pEndPoints->pszName, pszCmd);
                if(pszFind && !strcasecmp(pszFind, pszCmd))
                {
                    fprintf(stdout, "%s\n", pEndPoints->pszName);
                }
                pEndPoints = pEndPoints->pNext;
            }
        }
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
get_query_string(
    PPARSE_CONTEXT pContext,
    PREST_API_PARAM pApiParams,
    char **ppszQuery
    )
{
    uint32_t dwError = 0;
    int i = 0;
    PPARAM pParam = NULL;
    PREST_API_PARAM pApiParam = NULL;
    char *pszQuery = NULL;
    char *pszTemp = NULL;
    char *pszKeyValue = NULL;

    if(!pContext || !ppszQuery)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    for(pApiParam = pApiParams; pApiParam; pApiParam = pApiParam->pNext)
    {
        const char *pszName = pApiParam->pszName;

        dwError = get_param_by_name(pContext, pszName, &pParam);
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
            fprintf(stderr, "value required for param --%s\n", pszName);
            dwError = ENOENT;
            BAIL_ON_ERROR(dwError);
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
                      pszKeyValue,
                      pszQuery ? "&" : "");
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
rest_exec(
    PREST_API_DEF pApiDef,
    PPARSE_CONTEXT pContext
    )
{
    uint32_t dwError = 0;
    PREST_API_METHOD pMethod = NULL;
    char *pszUrl = NULL;
    char *pszEndPoint = NULL;
    RESTMETHOD nMethod = METHOD_GET;
    PREST_API_ENDPOINT pEndPoint = NULL;
    char *pszParams = NULL;

    if(!pApiDef || !pContext)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = get_method_spec(
                  pApiDef->pModules,
                  pContext->pszModule,
                  pContext->pszCmd,
                  &pEndPoint
                  );
    BAIL_ON_ERROR(dwError);

    pMethod = pEndPoint->pMethods[METHOD_GET];
    if(!pMethod)
    {
        fprintf(stderr,
                "GET method not found for command: %s\n",
                pContext->pszCmd);
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }

    dwError = get_query_string(pContext, pMethod->pParams, &pszParams);
    if(dwError == ENOENT)
    {
        dwError = 0;
    }
    BAIL_ON_ERROR(dwError);

    dwError = coapi_allocate_string_printf(
                  &pszUrl,
                  "http://%s%s%s%s",
                  pApiDef->pszHost,
                  pEndPoint->pszName,
                  pszParams ? "?" : "",
                  pszParams ? pszParams : "");
    BAIL_ON_ERROR(dwError);

    dwError = call_rest_method(pszUrl, pMethod, pContext->nVerbose);
    BAIL_ON_ERROR(dwError);

cleanup:
    SAFE_FREE_MEMORY(pszParams);
    SAFE_FREE_MEMORY(pszUrl);
    SAFE_FREE_MEMORY(pszEndPoint);
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
