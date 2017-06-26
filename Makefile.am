ACLOCAL_AMFLAGS = -I m4

APP_NAME=copenapi
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

tar-src:
	git archive --format=tar.gz --prefix=$(APP_NAME)-$(VERSION)/ -o $(APP_NAME)-$(VERSION).tar.gz HEAD

rpm: tar-src
	mkdir -p .rpm/SOURCES
	mkdir -p .rpm/BUILD
	mkdir -p .rpm/RPMS/x86_64
	-rm -rf .rpm/RPMS/$(APP_NAME)-*
	-rm -rf .rpm/SOURCES/$(APP_NAME)-*
	mv $(APP_NAME)-$(VERSION).tar.gz .rpm/SOURCES
	rpmbuild --quiet --nodeps --define="_topdir $(abs_top_builddir)/.rpm" \
	-bb $(APP_NAME).spec
	mkdir -p rpms
	mv .rpm/RPMS/x86_64/$(APP_NAME)-*$(VERSION)-*.x86_64.rpm rpms

clean-local:
	-rm -rf .rpm
	-rm -rf rpms
