SUBDIRS = pixmaps

AM_CFLAGS = @GNUTLS_FLAGS@ @LIBESMTP_FLAGS@ -Wall -export-dynamic

INCLUDES = \
	$(GNOME_APPLETS_CFLAGS) \
	-I$(top_srcdir) \
	-I$(includedir) \
	-I$(srcdir)/pixmaps \
	$(GNOME_INCLUDEDIR) \
	-DG_LOG_DOMAIN=\"gospy-applet\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DGLADEDIR=\""$(datadir)/gospy-applet"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = gospy-applet

gospy_applet_SOURCES = \
	gospy_applet.c gospy_applet.h \
	gospy_druid.c gospy_druid.h \
	gospy_properties.c gospy_properties.h \
	gospy_process.c gospy_process.h \
	gospy_gconf.c gospy_gconf.h

gospy_applet_LDADD = \
	$(GNOME_APPLETS_LIBS) @PACKAGE_LIBS@ @GNUTLS_LIBS@ @LIBESMTP_LIBS@

uidir 		= $(datadir)/gnome-2.0/ui
ui_DATA		= GNOME_GospyApplet.xml

@INTLTOOL_SCHEMAS_RULE@
schemasdir = @GCONF_SCHEMA_FILE_DIR@
schemas_in_files = gospy_applet.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

install-data-local:
if GCONF_SCHEMAS_INSTALL
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) \
		--makefile-install-rule $(schemas_DATA)
endif

serverdir = $(libdir)/bonobo/servers
server_in_files = GNOME_GospyApplet_Factory.server.in
server_in_in_files = $(server_in_files:.server.in=.server.in.in)
server_DATA = $(server_in_files:.server.in=.server)
@INTLTOOL_SERVER_RULE@

$(server_in_files): $(server_in_in_files)
	sed -e "s|\@BINDIR\@|$(bindir)|" $< > $@

EXTRA_DIST = \
	gospy_applet.schemas \
	$(schemas_in_files) \
	$(server_in_in_files) \
	$(ui_DATA)

DISTCLEANFILES = \
	$(schemas_DATA)

uninstall-local:
if GCONF_SCHEMAS_INSTALL
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) \
		--makefile-uninstall-rule $(schemas_DATA)
#		--makefile-uninstall-rule $(prefix)/share/gconf/schemas/$(schemas_DATA)
endif

clean-local:
	rm -f *~

distclean-local:

indent:
	if [ `which indent` ]; then \
	  chmod -R u+w ${srcdir} ; \
	  for f in `echo $(srcdir)/*.{c,h}`; do indent $$f; done \
	fi
	rm -f *~
