if ENABLE_JPEG
jpeg_LIB = $(top_builddir)/jpegutils/libeoc-jpegutils.la
endif

toolbar_LIB = $(top_builddir)/cut-n-paste/toolbar-editor/libtoolbareditor.la

noinst_LTLIBRARIES = libeoc.la

bin_PROGRAMS = eoc

headerdir = $(prefix)/include/eoc-@EOC_API_VERSION@/eoc
header_DATA = $(INST_H_FILES)

MARSHAL_OUTPUT = 	\
	eoc-marshal.h	\
	eoc-marshal.c

NOINST_H_FILES =			\
	eoc-application-internal.h	\
	eoc-session.h			\
	eoc-util.h			\
	eoc-pixbuf-util.h		\
	eoc-preferences-dialog.h	\
	eoc-config-keys.h		\
	eoc-image-jpeg.h		\
	eoc-image-private.h		\
	eoc-metadata-sidebar.h		\
	eoc-uri-converter.h		\
	eoc-metadata-reader.h		\
	eoc-metadata-reader-jpg.h	\
	eoc-metadata-reader-png.h	\
	eoc-save-as-dialog-helper.h	\
	eoc-print-image-setup.h         \
	eoc-print-preview.h             \
	eoc-print.h			\
	eoc-plugin-engine.h             \
	eoc-close-confirmation-dialog.h	\
	zoom.h

INST_H_FILES =				\
	eoc-application.h		\
	eoc-application-activatable.h	\
	eoc-debug.h			\
	eoc-window.h			\
	eoc-window-activatable.h	\
	eoc-sidebar.h			\
	eoc-properties-dialog.h		\
	eoc-error-message-area.h	\
	eoc-file-chooser.h		\
	eoc-statusbar.h			\
	eoc-thumb-nav.h			\
	eoc-transform.h			\
	eoc-image.h			\
	eoc-enums.h			\
	eoc-image-save-info.h		\
	eoc-scroll-view.h		\
	eoc-thumb-view.h		\
	eoc-list-store.h		\
	eoc-thumbnail.h			\
	eoc-job-queue.h			\
	eoc-clipboard-handler.h		\
	eoc-jobs.h

libeoc_c_files =			\
	eoc-application.c		\
	eoc-application-activatable.c	\
	eoc-session.c			\
	eoc-debug.c			\
	eoc-util.c			\
	eoc-pixbuf-util.c		\
	eoc-window.c			\
	eoc-window-activatable.c	\
	eoc-sidebar.c			\
	eoc-preferences-dialog.c	\
	eoc-properties-dialog.c		\
	eoc-error-message-area.c	\
	eoc-file-chooser.c		\
	eoc-statusbar.c			\
	eoc-thumb-nav.c			\
	eoc-transform.c			\
	eoc-image.c			\
	eoc-image-jpeg.c		\
	eoc-image-save-info.c		\
	eoc-scroll-view.c		\
	eoc-thumb-view.c		\
	eoc-list-store.c		\
	eoc-metadata-sidebar.c	\
	eoc-thumbnail.c			\
	eoc-job-queue.c			\
	eoc-jobs.c			\
	eoc-uri-converter.c		\
	eoc-metadata-reader.c		\
	eoc-metadata-reader-jpg.c	\
	eoc-metadata-reader-png.c	\
	eoc-save-as-dialog-helper.c	\
	eoc-print-image-setup.c         \
	eoc-print-preview.c           	\
	eoc-print.c			\
	eoc-close-confirmation-dialog.c \
	eoc-plugin-engine.c            	\
	eoc-clipboard-handler.c		\
	zoom.c

libeoc_la_SOURCES =			\
	$(libeoc_c_files)		\
	$(BUILT_SOURCES)		\
	$(NOINST_H_FILES)		\
	$(INST_H_FILES)

if HAVE_EXIF
INST_H_FILES +=				\
	eoc-exif-util.h

NOINST_H_FILES +=			\
	eoc-metadata-details.h

libeoc_c_files +=			\
	eoc-metadata-details.c  \
	eoc-exif-util.c
endif

if HAVE_EXEMPI
# We need to make sure eoc-metadata-details.h
# is only listed once in INST_H_FILES
# or the build will break with automake-1.11
if !HAVE_EXIF
NOINST_H_FILES +=				\
	eoc-metadata-details.h
endif !HAVE_EXIF
libeoc_la_SOURCES +=			\
	eoc-metadata-details.c
endif HAVE_EXEMPI

libeoc_la_CPPFLAGS =						\
	-I$(srcdir)						\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/jpegutils				\
	-I$(top_srcdir)/cut-n-paste/toolbar-editor		\
	-DG_LOG_DOMAIN=\"EOC\"					\
	-DEOC_DATA_DIR=\""$(pkgdatadir)"\"			\
	-DEOC_LOCALE_DIR=\""$(datadir)/locale"\"		\
	-DEOC_PLUGIN_DIR=\""$(libdir)/eoc/plugins"\"		\
	-DLIBDIR=\""$(libdir)"\"

libeoc_la_CFLAGS =						\
	$(EOC_CFLAGS)						\
	$(WARN_CFLAGS)

libeoc_la_LIBADD = \
	$(EOC_LIBS)

if HAVE_LCMS
libeoc_la_CFLAGS += 	\
	$(X11_CFLAGS)

libeoc_la_LIBADD += \
	$(X11_LIBS)
endif

eoc_SOURCES = \
	main.c \
	eoc-resources.c

eoc_CFLAGS = 							\
	-I$(top_srcdir)/cut-n-paste/toolbar-editor		\
	$(EOC_CFLAGS)						\
	-DEOC_DATA_DIR=\""$(pkgdatadir)"\"			\
	-DEOC_LOCALE_DIR=\""$(datadir)/locale"\"		\
	$(INTROSPECTION_CFLAGS)

eoc_LDADD = 				\
	libeoc.la			\
	$(EOC_LIBS)			\
	$(LIBJPEG)			\
	$(toolbar_LIB)			\
	$(jpeg_LIB)			\
	$(INTROSPECTION_LIBS)

BUILT_SOURCES = 			\
	eoc-enum-types.c		\
	eoc-enum-types.h		\
	eoc-resources.c			\
	$(MARSHAL_OUTPUT)

eoc_resources_deps = $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/data --generate-dependencies $(top_srcdir)/src/eoc.gresource.xml)

eoc-resources.c: $(top_srcdir)/src/eoc.gresource.xml $(eoc_resources_deps)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) "$<" --target=$@ \
         --sourcedir=$(top_srcdir)/data --c-name _eoc --generate-source

eoc-enum-types.h: eoc-enum-types.h.template $(INST_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN)(cd $(srcdir) && $(GLIB_MKENUMS) --template eoc-enum-types.h.template $(INST_H_FILES)) > $@

eoc-enum-types.c: eoc-enum-types.c.template $(INST_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN)(cd $(srcdir) && $(GLIB_MKENUMS) --template eoc-enum-types.c.template $(INST_H_FILES)) > $@

eoc-marshal.h: eoc-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN)$(GLIB_GENMARSHAL) $< --header --internal --prefix=eoc_marshal > $@

eoc-marshal.c: eoc-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN)$(GLIB_GENMARSHAL) $< --body --header --prefix=eoc_marshal > $@

EXTRA_DIST = 				\
	eoc.gresource.xml		\
	eoc-enum-types.h.template	\
	eoc-enum-types.c.template	\
	eoc-marshal.list

CLEANFILES = $(BUILT_SOURCES)

if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = Eoc-1.0.gir
INTROSPECTION_SCANNER_ARGS = -I$(top_srcdir) --warn-all $(libeoc_la_CPPFLAGS)

Eoc-1.0.gir: eoc
Eoc_1_0_gir_INCLUDES = Gtk-3.0
Eoc_1_0_gir_FILES = $(INST_H_FILES) $(libeoc_c_files) $(BUILT_SOURCES)
Eoc_1_0_gir_VERSION = 1.0
Eoc_1_0_gir_PROGRAM = $(builddir)/eoc
Eoc_1_0_gir_NAMESPACE = Eoc
Eoc_1_0_gir_CFLAGS = $(EOC_CFLAGS)
girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
