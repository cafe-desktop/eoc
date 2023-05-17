NULL =

if USE_INTERNAL_THUMBNAILER

bin_PROGRAMS = eoc-thumbnailer
eoc_thumbnailer_SOURCES = \
	eoc-thumbnailer.c	\
	$(NULL)
eoc_thumbnailer_CPPFLAGS = \
	-I$(top_srcdir)		\
	-I$(top_builddir)	\
	$(AM_CPPFLAGS)		\
	$(NULL)
eoc_thumbnailer_CFLAGS = \
	$(THUMBNAILER_CFLAGS)	\
	$(DISABLE_DEPRECATED)	\
	$(WARN_CFLAGS)		\
	$(AM_CFLAGS)		\
	$(NULL)
eoc_thumbnailer_LDFLAGS = \
	$(AM_LDFLAGS)		\
	-lm			\
	$(THUMBNAILER_LIBS)	\
	$(NULL)

endif

thumbnailerdir = $(datadir)/thumbnailers
thumbnailer_DATA = eoc-thumbnailer.thumbnailer

EXTRA_DIST = \
	eoc-thumbnailer.thumbnailer.in \
	$(NULL)

CLEANFILES = \
	$(thumbnailer_DATA)	\
	$(NULL)

-include $(top_srcdir)/git.mk
