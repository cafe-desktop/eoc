AUTOMAKE_OPTIONS = subdir-objects

pluginsdir = $(libdir)/eoc/plugins

AM_CPPFLAGS = \
	-I$(top_srcdir)/src 				\
	$(EOC_CFLAGS) 					\
	$(WARN_CFLAGS)					\
	-DEOC_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

AM_LDFLAGS = -avoid-version -module

plugins_LTLIBRARIES = \
	fullscreen/libfullscreen.la  \
	reload/libreload.la

plugins_in_files = \
	fullscreen/fullscreen.plugin.desktop.in  \
	reload/reload.plugin.desktop.in

# Fullscreen plugin
fullscreen_libfullscreen_la_SOURCES = \
	fullscreen/eoc-fullscreen-plugin.h  \
	fullscreen/eoc-fullscreen-plugin.c

fullscreen_libfullscreen_la_LIBADD = $(EOC_LIBS)

# Reload plugin
reload_libreload_la_SOURCES = \
	reload/eoc-reload-plugin.h  \
	reload/eoc-reload-plugin.c

reload_libreload_la_LIBADD = $(EOC_LIBS)

# Statusbar Date Plugin
if HAVE_EXIF
plugins_LTLIBRARIES += statusbar-date/libstatusbar-date.la

statusbar_date_libstatusbar_date_la_SOURCES = \
	statusbar-date/eoc-statusbar-date-plugin.h  \
	statusbar-date/eoc-statusbar-date-plugin.c

statusbar_date_libstatusbar_date_la_LIBADD = $(EOC_LIBS)
plugins_in_files += statusbar-date/statusbar-date.plugin.desktop.in
endif

# Generate plugin info files

plugins_DATA = $(plugins_in_files:.plugin.desktop.in=.plugin)

%.plugin: %.plugin.desktop.in
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

DISTCLEANFILES = $(plugins_DATA)
CLEANFILES = $(plugins_DATA)

EXTRA_DIST = $(plugins_in_files)

-include $(top_srcdir)/git.mk
