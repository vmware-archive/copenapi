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

//Illustrates loading of an apispec from file and performing a
//print out of the apis

#include <stdio.h>
#include <copenapi/copenapi.h>

int
main(
    int argc,
    char **argv
    )
{
    int dwError = 0;
    PREST_API_DEF pApiDef = NULL;

    dwError = coapi_load_from_file("../tests/test.json", &pApiDef);
    if(dwError)
    {
        goto error;
    }

    coapi_print_api_def(pApiDef);

cleanup:
    coapi_free_api_def(pApiDef);
    return dwError;

error:
    fprintf(stdout, "Error: %d\n", dwError);
    goto cleanup;
}
