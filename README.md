

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
2. cmd line client (cli/copenapi_cli) - [cli how to](#cli-how-to)
3. library - [api how to](#api-how-to)

## cli how to
copenapi_cli can work with swagger specs for eg: [swagger petstore json](http://petstore.swagger.io/v2/swagger.json)

Use copenapi_cli against the petstore swagger spec. It works like a customized cli for your spec.

get the petstore swagger spec (cli does not support external urls for this)
~~~
[ ~/pet ]# wget http://petstore.swagger.io/v2/swagger.json

#create the .copenapi file or you can use --apispec to point to swagger.json on disk.
[ ~/pet ]# cat >> .copenapi <<-EOF
[default]
apispec=$PWD/swagger.json
EOF

~~~

Try the cli. Note copenapi_cli maps tags as modules.
~~~
[ ~/pet ]# copenapi_cli
The following modules are supported.
 user            : Operations about user
 store           : Access to Petstore orders
 pet             : Everything about your Pets
To get help on a module, do <module> --help.
To get help on a module's command, do <module> <command> --help.
~~~

Inspect what is provided by a module
~~~
[ ~/pet ]# copenapi_cli pet --help
Commands under module pet :
pet             Update an existing pet
pet             Add a new pet to the store
findByStatus    Finds Pets by status
findByTags      Finds Pets by tags
{petId}         Find pet by ID
{petId}         Updates a pet in the store with form data
{petId}         Deletes a pet
uploadImage     uploads an image

To get help on a command, do pet <command> --help.
~~~

Get help on a method in a module / tag
~~~
[ ~/pet ]# copenapi_cli pet findByStatus --help

Name : /v2/pet/findByStatus

Method: get
Summary : Finds Pets by status
Description : Multiple status values can be provided with comma separated strings
Param1 : status - Required
~~~

Invoke the method. Note GET is the default.
If you have multiple methods with the same name, ambiguity is resolved
in a way that will not surprise you. For eg: try copenapi_cli pet pet
You can use -v to get all the details of the communication.
~~~
[ ~/pet ]# copenapi_cli pet findByStatus
Parameter status is required. Specify as --status

[ ~/pet ]# copenapi_cli pet findByStatus --status 1
[{"id":9205436248879947591,"category":{"id":0,"name":"打死你"},"name":"doggie","photoUrls":["string"],"tags":[{"id":0,"name":"二哈"}],"status":"1"}]
~~~

## api how to

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
