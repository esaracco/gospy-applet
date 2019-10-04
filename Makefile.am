SUBDIRS = gospy-applet.glade src help po pixmaps

Applicationsdir = $(datadirs)/gnome/apps/Applications

EXTRA_DIST = \
        intltool-merge.in \
        intltool-extract.in \
        intltool-update.in \
	xmldocs.make \
	omf.make

dist-hook: indent

clean-local:
	rm -f *~ $(PACKAGE)-*.tar.gz

distclean-local:
	rm -rf aclocal.m4 autom4te.cache config.guess config.sub \
		stamp-h.in intltool-extract intltool-update intltool-merge

indent:
	(cd src && make indent)

ACLOCAL_AMFLAGS = -I m4
