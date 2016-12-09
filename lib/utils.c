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

uint32_t
coapi_file_read_all_text(
    const char *pszFileName,
    char **ppszText
    )
{
    uint32_t dwError = 0;
    FILE *fp = NULL;
    char *pszText = NULL;
    int nLength = 0;
    int nBytesRead = 0;

    if(!pszFileName || !ppszText)
    {
        dwError = EINVAL;
        BAIL_ON_ERROR(dwError);
    }

    fp = fopen(pszFileName, "r");
    if(!fp)
    {
        dwError = ENOENT;
        BAIL_ON_ERROR(dwError);
    }
    fseek(fp, 0, SEEK_END);
    nLength = ftell(fp);

    dwError = coapi_allocate_memory(nLength + 1, (void **)&pszText);
    BAIL_ON_ERROR(dwError);

    if(fseek(fp, 0, SEEK_SET))
    {
        dwError = errno;
        BAIL_ON_ERROR(dwError);
    }

    nBytesRead = fread(pszText, 1, nLength, fp);
    if(nBytesRead != nLength)
    {
        dwError = EBADFD;
        BAIL_ON_ERROR(dwError);
    }

    *ppszText = pszText;
cleanup:
    if(fp)
    {
        fclose(fp);
    }
    return dwError;

error:
    if(ppszText)
    {
        *ppszText = NULL;
    }
    SAFE_FREE_MEMORY(pszText);
    goto cleanup;
}

