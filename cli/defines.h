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

#define BOLD "\033[1m\033[30m"
#define RESET   "\033[0m"
#define COPENAPI_CONFIG_FILE ".copenapi"

#define COPENAPI_CLI_SHOW_HELP 128

#define ERROR_COPENAPI_CLI_BASE        1000
#define ERROR_COPENAPI_CLI_CURL_BASE   1300
#define ERROR_COPENAPI_CLI_CURL_END    1400

#define HTTP_OK  200

//cmd line client options
#define OPT_BASEURL  "baseurl"
#define OPT_USER     "user"
#define OPT_APISPEC  "apispec"
#define OPT_VERBOSE  "verbose"
#define OPT_INSECURE "insecure"
#define OPT_NETRC    "netrc"
#define OPT_HELP     "help"
#define OPT_REQUEST  "request"

#define CONFIG_KEY_APISPEC     "apispec"
#define CONFIG_KEY_BASEURL     "baseurl"
#define CONFIG_KEY_HEADER      "header."
#define CONFIG_KEY_INSECURE    "insecure"

#define BAIL_ON_CURL_ERROR(dwError) \
    do {                                                           \
        if (dwError)                                               \
        {                                                          \
            dwError += ERROR_COPENAPI_CLI_CURL_BASE;               \
            goto error;                                            \
        }                                                          \
    } while(0)
