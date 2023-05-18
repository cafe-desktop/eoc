SUBDIRS = pixmaps icons

DESKTOP_IN_FILES= eoc.desktop.in.in
DESKTOP_FILES= $(DESKTOP_IN_FILES:.desktop.in.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(DESKTOP_FILES)
$(desktop_DATA): $(DESKTOP_IN_FILES)
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword= --keyword=Name --keyword=Comment --keyword=Keywords --template $< -d $(top_srcdir)/po -o $@

appdatadir = $(datadir)/metainfo
appdata_in_files = eoc.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
$(appdata_DATA): $(appdata_in_files)
	$(AM_V_GEN) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

gsettings_ENUM_NAMESPACE = org.cafe.eoc
gsettings_ENUM_FILES = $(top_srcdir)/src/eoc-scroll-view.h	\
                       $(top_srcdir)/src/eoc-window.h

gsettings_SCHEMAS = org.cafe.eoc.gschema.xml
@GSETTINGS_RULES@

uidir = $(pkgdatadir)
ui_DATA = \
	eoc-toolbar.xml

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = eoc.pc

EXTRA_DIST = \
	$(ui_DATA) \
	eoc.css			\
	eoc-image-properties-dialog.ui	\
	eoc-multiple-save-as-dialog.ui	\
	eoc-preferences-dialog.ui \
	eoc-ui.xml		\
	metadata-sidebar.ui	\
	eoc.about		\
	$(DESKTOP_IN_FILES) \
	$(appdata_in_files)

CLEANFILES = \
        $(appdata_DATA)

DISTCLEANFILES = \
        $(DESKTOP_FILES) \
        $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
