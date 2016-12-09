bin_PROGRAMS = coapi_cli

AM_CFLAGS += $(LIBCURL_CFLAGS)

coapi_cli_CPPFLAGS = -I$(top_srcdir)/include

coapi_cli_SOURCES = \
    help.c \
    main.c \
    param.c \
    parseargs.c \
    restclient.c \
    utils.c

coapi_cli_LDADD =  \
    $(top_builddir)/lib/libcopenapi.la \
    @LIBCURL_LIBS@
