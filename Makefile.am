
CLEANFILES =
EXTRA_DIST =
pkginclude_HEADERS = include/copenapi.h include/copenapitypes.h
pkgconfigdir = $(libdir)/pkgconfig

SUBDIRS = \
    common \
    lib \
    cli

pkgconfig_DATA = copenapi.pc
copenapi.pc: $(top_srcdir)/copenapi.pc.in
	./config.status --file=${subdir}/copenapi.pc:${subdir}/copenapi.pc.in
CLEANFILES += copenapi.pc
EXTRA_DIST += copenapi.pc.in
