

# copenapi

## Overview
copenapi (C open api) is an openapi spec parser library in C. It includes a general purpose
command line tool to browse an openapi spec file. copenapi supports openapi spec files in
json format.

## Try it out

### Prerequisites

* Jansson
* libcurl

### Build & Run

1. autoreconf -mif && ./configure && make
2. cmd line client - run cli/copenapi_cli --apispec <openapi spec.json>
3. library - see documentation below

## Documentation

To load an api spec from json file and map implementation, follow the sample code below

    #include <copenapi/copenapi.h>

    REST_MODULE _module1_rest_module[] =
    {
       {
           "/v1/module1/version",
           {module1_rest_get_version, NULL, NULL, NULL}
       }
    }

    //Module registration map
    MODULE_REG_MAP stRegMap[] =
    {
        {"module1", module1_rest_module},
        {NULL, NULL}
    };
    PREST_API_DEF pApiDef = NULL;
    //load api spec
    coapi_load_from_file("/home/user/apispec.json", &pApiDef);
    //map implementation
    coapi_map_api_impl(pApiDef, stRegMap);

At this point, pApiDef is populated with all the methods from the spec file and their implementations in source.
In this example, the entry point /v1/module1/version is mapped to function module1_rest_get_version.
You can now hook this up to a REST engine and handle incoming calls with spec driven
parameter validation, type validation, error messages and error codes.

## Releases & Major Branches
Initial release 0.0.1 alpha

## Contributing

The copenapi project team welcomes contributions from the community. If you wish to contribute code and you have not
signed our contributor license agreement (CLA), our bot will update the issue when you open a Pull Request. For any
questions about the CLA process, please refer to our [FAQ](https://cla.vmware.com/faq). For more detailed information,
refer to [CONTRIBUTING.md](CONTRIBUTING.md).

## License
copenapi is available under the [Apache 2 license](LICENSE).
