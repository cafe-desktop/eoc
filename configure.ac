AC_PREREQ(2.59)
AC_CONFIG_MACRO_DIR([m4])

m4_define(eoc_major_version, 1)
m4_define(eoc_minor_version, 24)
m4_define(eoc_micro_version, 1)
m4_define(eoc_version, eoc_major_version.eoc_minor_version.eoc_micro_version)

AC_INIT([eoc], eoc_version, [http://www.cafe-desktop.org], [eoc])
AM_INIT_AUTOMAKE([1.9 foreign no-dist-gzip dist-xz check-news])

# Support silencing the build output if supported (automake-1.11+)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_DEFINE(EOC_MAJOR_VERSION, eoc_major_version, [EOC major version])
AC_SUBST(EOC_MAJOR_VERSION,  eoc_major_version)
AC_DEFINE(EOC_MINOR_VERSION, eoc_minor_version, [EOC minor version])
AC_SUBST(EOC_MINOR_VERSION,  eoc_minor_version)
AC_DEFINE(EOC_MICRO_VERSION, eoc_micro_version, [EOC micro version])
AC_SUBST(EOC_MICRO_VERSION,  eoc_micro_version)

EOC_API_VERSION=2.20
AC_SUBST(EOC_API_VERSION)

AC_CONFIG_HEADERS([config.h])

AM_MAINTAINER_MODE

CAFE_COMMON_INIT
CAFE_DEBUG_CHECK
CAFE_COMPILE_WARNINGS
CAFE_MAINTAINER_MODE_DEFINES

# Disable building static archives by default
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_PROG_CC
AM_PROG_CC_C_O
AC_PATH_PROG([GLIB_COMPILE_RESOURCES], [glib-compile-resources])
AC_PATH_PROG([GLIB_GENMARSHAL], [glib-genmarshal])
AC_PATH_PROG([GLIB_MKENUMS],[glib-mkenums])

GTK_DOC_CHECK(1.9)

AC_CHECK_FUNCS(strptime)

# ****************************************************************
# Support for nl_langinfo (_NL_MEASUREMENT_MEASUREMENT) (optional)
# ****************************************************************
AC_MSG_CHECKING([for _NL_MEASUREMENT_MEASUREMENT])
AC_TRY_LINK([#include <langinfo.h>], [
char c;
c = *((unsigned char *)  nl_langinfo(_NL_MEASUREMENT_MEASUREMENT));
], have_langinfo_measurement=yes, have_langinfo_measurement=no)
AC_MSG_RESULT($have_langinfo_measurement)
if test "$have_langinfo_measurement" = "yes"; then
  AC_DEFINE([HAVE__NL_MEASUREMENT_MEASUREMENT], [1],
            [Define if _NL_MEASUREMENT_MEASUREMENT is available])
fi

# ***********
# Translation
# ***********
AM_GNU_GETTEXT_VERSION([0.19.8])
AM_GNU_GETTEXT_REQUIRE_VERSION([0.19.8])
AM_GNU_GETTEXT([external])

GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package.])

AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)

# ***************
# cafe-doc-utils
# ***************

PKG_PROG_PKG_CONFIG

YELP_HELP_INIT

# **********
# CAFE Libs
# **********

GLIB_REQUIRED=2.52.0
GIO_REQUIRED=2.50.0
CTK_REQUIRED=3.22.0
CAFE_DESKTOP_REQUIRED=1.17.0
CDKPIXBUF_REQUIRED=2.36.5
SHARED_MIME_INFO_REQUIRED=0.20
EXEMPI_REQUIRED=1.99.5
LIBPEAS_REQUIRED=1.8.0
MAGICK_REQUIRED=6.2.6

PKG_CHECK_MODULES(GMODULE, gmodule-2.0, [GMODULE_ADD="gmodule-2.0"],[GMODULE_ADD=""])
EOC_MODULES="ctk+-3.0 >= $CTK_REQUIRED \
             glib-2.0 >= $GLIB_REQUIRED \
             $GMODULE_ADD \
             gio-2.0 >= $GLIB_REQUIRED \
             gio-unix-2.0 >= $GLIB_REQUIRED \
             gthread-2.0 >= $GLIB_REQUIRED \
             cafe-desktop-2.0 >= $CAFE_DESKTOP_REQUIRED \
             gdk-pixbuf-2.0 >= $CDKPIXBUF_REQUIRED \
             gio-2.0 >= $GIO_REQUIRED \
             ctk+-unix-print-3.0 >= $CTK_REQUIRED \
             shared-mime-info >= $SHARED_MIME_INFO_REQUIRED \
             libpeas-1.0 >= $LIBPEAS_REQUIRED \
             libpeas-ctk-1.0 >= $LIBPEAS_REQUIRED"

# Introspection
GOBJECT_INTROSPECTION_CHECK([0.9.3])

if test "$found_introspection" = "yes"; then
	have_introspection=yes
	AC_DEFINE([HAVE_INTROSPECTION], [1], [Define to enable GObject Introspection])
else
	have_introspection=no
fi

AM_CONDITIONAL(HAVE_INTROSPECTION, test "x$have_introspection" = xyes)

# ***************
# ZLIB (required)
# ***************

have_zlib=yes
AC_CHECK_HEADERS([zlib.h],
	[AC_CHECK_LIB([z], [inflate],
		[AC_CHECK_LIB([z], [crc32], [], [have_zlib=no])],
		[have_zlib=no])],
	[have_zlib=no])

if test x$have_zlib = xno; then
	AC_MSG_ERROR([No sufficient zlib library found on your system.])
fi

# ***************
# EXIF (optional)
# ***************

LIBEXIF_REQUIRED=0.6.14

AC_ARG_WITH([libexif], AC_HELP_STRING([--without-libexif], [disable EXIF support]))
have_exif=no
if test x$with_libexif != xno; then
    PKG_CHECK_MODULES(EXIF, libexif >= $LIBEXIF_REQUIRED, have_exif=yes, have_exif=no)
fi
if test "x$have_exif" = "xyes"; then
  AC_DEFINE(HAVE_EXIF, 1, [EXIF Support.])
  EOC_MODULES="$EOC_MODULES libexif >= $LIBEXIF_REQUIRED"
fi

AM_CONDITIONAL([HAVE_EXIF], [test "x$have_exif" = "xyes"])

# *********************
# Little CMS (optional)
# *********************

AC_ARG_WITH([cms], AC_HELP_STRING([--without-cms], [disable colour management support]))
have_lcms=no
if test x$with_cms != xno; then
    PKG_CHECK_MODULES(LCMS, lcms2, have_lcms=yes, have_lcms=no)
fi
if test "x$have_lcms" = "xyes"; then
  AC_DEFINE(HAVE_LCMS, 1, [Little CMS Support.])
  EOC_MODULES="$EOC_MODULES lcms2"
fi

AM_CONDITIONAL([HAVE_LCMS], [test "x$have_lcms" = "xyes"])

# *********************
# Exempi (optional)
# *********************
AC_ARG_WITH(xmp, AC_HELP_STRING([--without-xmp], [disable special XMP support]))
have_exempi=no
if test x$with_xmp != xno; then
    PKG_CHECK_MODULES(EXEMPI, exempi-2.0 >= $EXEMPI_REQUIRED, have_exempi=yes, have_exempi=no)
fi
if test "x$have_exempi" = "xyes"; then
  AC_DEFINE(HAVE_EXEMPI,1, [XMP support.])
  EOC_MODULES="$EOC_MODULES exempi-2.0 >= $EXEMPI_REQUIRED"
fi

AM_CONDITIONAL([HAVE_EXEMPI], [test "x$have_exempi" = "xyes"])

# Build EXIF-related documentation if we really have support for it
if test "x$have_exif" = "xyes" || test "x$have_exempi" = "xyes"; then
  EOC_DOC_EXIF_START=""
  EOC_DOC_EXIF_STOP=""
else
  EOC_DOC_EXIF_START="<!--"
  EOC_DOC_EXIF_STOP="-->"
fi

AC_SUBST([EOC_DOC_EXIF_START])
AC_SUBST([EOC_DOC_EXIF_STOP])

# ********************
# Jpeg (semi-optional)
# ********************

AC_ARG_WITH(libjpeg, AC_HELP_STRING([--without-libjpeg], [disable special JPEG support]))
  have_jpeg=no
  have_libjpeg_80=no
  if test x$with_libjpeg != xno && test -z "$LIBJPEG"; then
    AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,
      have_jpeg=yes,
      have_jpeg=no
      AC_MSG_WARN(*** JPEG loader will not be built (JPEG library not found) ***))
    if test "$have_jpeg" = yes; then
      AC_MSG_CHECKING([for jpeglib.h])
      AC_TRY_CPP(
[#include <stdio.h>
#undef PACKAGE
#undef VERSION
#undef HAVE_STDLIB_H
#include <jpeglib.h>],
        have_jpeg=yes,
        have_jpeg=no)
      AC_MSG_RESULT($have_jpeg)
      if test "$have_jpeg" = yes; then
        LIBJPEG='-ljpeg'
	AC_DEFINE(HAVE_JPEG, 1, [Jpeg Support.])
        AC_DEFINE(HAVE_LIBJPEG, 1, [libjpeg is Present.])
        AC_CHECK_LIB(jpeg, jpeg_simple_progression,
          AC_DEFINE(HAVE_PROGRESSIVE_JPEG, 1, [Jpeg Progressive Saving Support.]),
          AC_MSG_WARN(JPEG library does not support progressive saving.))
	AC_MSG_CHECKING([[libjpeg version is 8 or greater]])
	AC_PREPROC_IFELSE([ AC_LANG_SOURCE(
			  [[
#include <stdio.h>
#include <jpeglib.h>
#if JPEG_LIB_VERSION < 80
#error "wrong version"
#endif
int main(int c, char**v) { return 0; }
			  ]]) ],
			  [AC_MSG_RESULT(yes)
			   have_libjpeg_80=yes],
		          [AC_MSG_RESULT(no)
		           have_libjpeg_80=no])

      else
          AC_MSG_WARN(*** JPEG loader will not be built (JPEG header file not found) ***)
      fi
    fi
  fi

  if test x$with_libjpeg != xno && test -z "$LIBJPEG"; then
     AC_MSG_ERROR([
*** Checks for JPEG loader failed. You can build without it by passing
*** --without-libjpeg to configure but some programs using CTK+ may
*** not work properly])
  fi
AC_SUBST(LIBJPEG)
AM_CONDITIONAL(ENABLE_JPEG, test x$have_jpeg = xyes)
AM_CONDITIONAL(HAVE_LIBJPEG_80, test "x$have_libjpeg_80" = xyes)

# ************************************
# libXML2 (required for toolbareditor)
# ************************************

LIBXML2_REQUIRED=2.0
PKG_CHECK_MODULES(LIBXML2, [libxml-2.0 >= $LIBXML2_REQUIRED])

# ***************
# RSVG (optional for scaling svg image)
# ***************

LIBRSVG_REQUIRED=2.36.2

AC_ARG_WITH([librsvg], AC_HELP_STRING([--without-librsvg], [disable RSVG support]))
have_rsvg=no
if test x$with_librsvg != xno; then
    PKG_CHECK_MODULES(RSVG, librsvg-2.0 >= $LIBRSVG_REQUIRED, have_rsvg=yes, have_rsvg=no)
fi
if test "x$have_rsvg" = "xyes"; then
  AC_DEFINE(HAVE_RSVG, 1, [RSVG Support.])
  EOC_MODULES="$EOC_MODULES librsvg-2.0 >= $LIBRSVG_REQUIRED"
else
  AC_WARN([LIBRSVG support disabled, svg files will not scale])
fi

AM_CONDITIONAL([HAVE_RSVG], [test "x$have_rsvg" = "xyes"])

# ***********************
# THUMBNAILER CFLAGS/LIBS
# ***********************

AC_ARG_ENABLE(thumbnailer,
	[AS_HELP_STRING([--enable-thumbnailer],
			[Enable thumbnailer])],
	[enable_thumbnailer=$enableval],
	[enable_thumbnailer=no])
AM_CONDITIONAL(ENABLE_THUMBNAILER, test x$enable_thumbnailer != xno)

AC_ARG_WITH(gdk-pixbuf-thumbnailer,
	[AS_HELP_STRING([--without-gdk-pixbuf-thumbnailer],
		[Disable the use of gdk-pixbuf-thumbnailer])],
	[],
	[with_gdk_pixbuf_thumbnailer=yes])
AM_CONDITIONAL(USE_INTERNAL_THUMBNAILER, test x$with_gdk_pixbuf_thumbnailer != xyes)

if test "x$enable_thumbnailer" != "xno"; then
	if test "x$with_gdk_pixbuf_thumbnailer" = "xyes"; then
		AC_PATH_PROG([GDK_PIXBUF_THUMBNAILER],[gdk-pixbuf-thumbnailer],[])
		if test -z "$GDK_PIXBUF_THUMBNAILER"; then
			AC_MSG_ERROR([gdk-pixbuf-thumbnailer not found])
		fi
		eoc_thumbnailer="$GDK_PIXBUF_THUMBNAILER"
	else
		PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED)
		PKG_CHECK_MODULES(GIO, gio-2.0 >= $GLIB_REQUIRED)
		PKG_CHECK_MODULES(MAGICK, MagickWand >= $MAGICK_REQUIRED)

		CPPFLAGS_save="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS $MAGICK_CFLAGS"
		AC_CHECK_HEADER([wand/MagickWand.h],
			[have_im6="yes"],
			[AC_CHECK_HEADER([MagickWand/MagickWand.h], [have_im7="yes"])])
		CPPFLAGS="$CPPFLAGS_save"

		if test "x$have_im6" = "xyes"; then
			 AC_DEFINE(HAVE_IMAGEMAGICK6,1, [Have ImageMagick 6])
		fi
		if test "x$have_im7" = "xyes"; then
			AC_DEFINE(HAVE_IMAGEMAGICK7,1, [Have ImageMagick 7])
		fi

		THUMBNAILER_CFLAGS="$CFLAGS $GLIB_CFLAGS $GIO_CFLAGS $MAGICK_CFLAGS"
		AC_SUBST(THUMBNAILER_CFLAGS)

		THUMBNAILER_LIBS="$LIBS $GLIB_LIBS $GIO_LIBS $MAGICK_LIBS"
		AC_SUBST(THUMBNAILER_LIBS)
		eoc_thumbnailer="eoc-thumbnailer"
	fi

	EOC_THUMBNAILER="$eoc_thumbnailer"
	AC_SUBST(EOC_THUMBNAILER)
else
	eoc_thumbnailer="no"
fi

# ****************
# CFLAGS/LIBS init
# ****************

PKG_CHECK_MODULES(EOC, $EOC_MODULES)
AC_SUBST(EOC_CFLAGS)
AC_SUBST(EOC_LIBS)

# *******************************************************
# libX11 (required for TotemScrSaver and Color Profiling)
# *******************************************************

# Check CDK rendering target
cdk_target_has_x11=`$PKG_CONFIG --variable=targets cdk-3.0 | grep x11`

if test "x$have_lcms" = "xyes" || test "$cdk_target_has_x11" != ""; then
	PKG_CHECK_MODULES(X11, [x11], have_x11=yes, have_x11=no)

	if test "x$have_x11" != "xyes"; then
		# Fallback for X11 releases without .pc-files
		AC_PATH_XTRA
		if test x$no_x = xyes ; then
			AC_MSG_ERROR([X development libraries (libX11) not found])
		else
			X11_LIBS="$X_PRE_LIBS $X_LIBS -lX11 $X_EXTRA_LIBS"
			AC_SUBST(X11_LIBS)
			have_x11=yes
		fi
	fi
fi

# *****
# GSettings
# *****

GLIB_GSETTINGS

AC_SEARCH_LIBS([floor],[m])

AC_CONFIG_FILES([
Makefile
src/Makefile
man/Makefile
jpegutils/Makefile
cut-n-paste/Makefile
cut-n-paste/toolbar-editor/Makefile
help/Makefile
po/Makefile.in
data/Makefile
data/eoc.pc
data/eoc.desktop.in
data/org.cafe.eoc.gschema.xml
data/pixmaps/Makefile
data/icons/Makefile
doc/Makefile
doc/reference/Makefile
doc/reference/version.xml
doc/reference/eoc-docs.sgml
plugins/Makefile
thumbnailer/Makefile
thumbnailer/eoc-thumbnailer.thumbnailer
])

AC_OUTPUT

echo "
Configure summary:

	Source code location .......:  ${srcdir}
	Compiler ...................:  ${CC}
	Extra Compiler Warnings ....:  ${WARN_CFLAGS}

	EXIF support ...............:  ${have_exif}
	XMP support ................:  ${have_exempi}
	JPEG support ...............:  ${have_jpeg}
	RSVG support ...............:  ${have_rsvg}
	Colour management support ..:  ${have_lcms}
	GObject Introspection.......:  ${have_introspection}
	Thumbnailer.................:  ${eoc_thumbnailer}
"
