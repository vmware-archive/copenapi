bin_PROGRAMS = copenapi_cli

AM_CFLAGS += $(LIBCURL_CFLAGS)

copenapi_cli_CPPFLAGS = -I$(top_srcdir)/include

copenapi_cli_SOURCES = \
    help.c \
    main.c \
    parseargs.c \
    parsecmdargs.c \
    restclient.c \
    utils.c

copenapi_cli_LDADD =  \
    $(top_builddir)/lib/libcopenapi.la \
    @LIBCURL_LIBS@
