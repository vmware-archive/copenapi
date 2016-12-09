#pragma once

//memory.c
uint32_t
coapi_allocate_memory(
    size_t size,
    void** ppMemory
    );

void
coapi_free_memory(
    void* pMemory
    );

//strings.c
uint32_t
coapi_allocate_string(
    const char* pszSrc,
    char** ppszDest
    );

uint32_t
coapi_allocate_string_printf(
    char** ppszDst,
    const char* pszFmt,
    ...
    );

void
coapi_free_string_array_with_count(
    char **ppszArray,
    int nCount
    );

//utils.c
uint32_t
dup_argv(
    int argc,
    char* const* argv,
    char*** argvDup
    );

const char *
ltrim(
    const char *pszStr
    );

const char *
rtrim(
    const char *pszStart,
    const char *pszEnd
    );

//configreader.c
void
print_config_data(
    PCONF_DATA pData
    );

uint32_t
read_config_file_custom(
    const char *pszFile,
    const int nMaxLineLength,
    PFN_CONF_SECTION_CB pfnSectionCB,
    PFN_CONF_KEYVALUE_CB pfnKeyValueCB,
    PCONF_DATA *ppData
    );

uint32_t
read_config_file(
    const char *pszFile,
    const int nMaxLineLength,
    PCONF_DATA *ppData
    );

uint32_t
config_get_section(
    PCONF_DATA pData,
    const char *pszGroup,
    PCONF_SECTION *ppSection
    );

void
free_config_data(
    PCONF_DATA pData
    );
