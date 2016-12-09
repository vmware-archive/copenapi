lib_LTLIBRARIES = libcopenapi.la

AM_CFLAGS += $(JANSSON_CFLAGS)

libcopenapi_la_CPPFLAGS = \
    -I$(top_srcdir)/include
    
libcopenapi_la_SOURCES = \
    api.c \
    jsonutils.c \
    restapidef.c \
    utils.c

libcopenapi_la_LDFLAGS =  \
    $(top_builddir)/common/libcommon.la \
    @JANSSON_LIBS@
