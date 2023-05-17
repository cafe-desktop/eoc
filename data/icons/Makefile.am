
actioniconsdir = $(pkgdatadir)/icons/hicolor
appiconsdir = $(datadir)/icons/hicolor

nobase_dist_actionicons_DATA = \
	16x16/actions/eoc-image-collection.png \
	16x16/actions/eoc-plugin.png \
	16x16/actions/slideshow-play.png \
	22x22/actions/eoc-image-collection.png \
	22x22/actions/eoc-plugin.png \
	22x22/actions/slideshow-play.png \
	24x24/actions/eoc-image-collection.png \
	24x24/actions/slideshow-play.png \
	32x32/actions/eoc-image-collection.png \
	32x32/actions/eoc-plugin.png \
	32x32/actions/slideshow-play.png \
	48x48/actions/slideshow-play.png
	scalable/actions/eoc-image-collection.svg \
	scalable/actions/eoc-plugin.svg \
	scalable/actions/slideshow-play.svg

nobase_dist_appicons_DATA = \
	16x16/apps/eoc.png \
	22x22/apps/eoc.png \
	24x24/apps/eoc.png \
	32x32/apps/eoc.png \
	scalable/apps/eoc.svg

noinst_DATA = \
	16x16/actions/eoc-plugin.svg \
	16x16/actions/slideshow-play.svg \
	16x16/apps/eoc.svg \
	22x22/actions/eoc-plugin.svg \
	22x22/actions/slideshow-play.svg \
	22x22/apps/eoc.svg \
	32x32/actions/eoc-image-collection.svg \
	32x32/actions/eoc-plugin.svg \
	32x32/actions/slideshow-play.svg \
	32x32/apps/eoc.svg

EXTRA_DIST = $(noinst_DATA)

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

-include $(top_srcdir)/git.mk
