@YELP_HELP_RULES@

HELP_ID = eoc
HELP_FILES = index.docbook legal.xml
HELP_MEDIA = 					\
	figures/eoc_save_as_window.png		\
	figures/eoc_start_window.png		\
	figures/eoc_toolbar_editor_window.png

# Add linguas to be ignored, e.g. IGNORE_HELP_LINGUAS = ca de es fr
IGNORE_HELP_LINGUAS =
HELP_LINGUAS = $(if $(IGNORE_HELP_LINGUAS), \
	$(filter-out $(IGNORE_HELP_LINGUAS),$(subst /,,$(dir $(wildcard */*.po)))), \
	$(subst /,,$(dir $(wildcard */*.po))) )

-include $(top_srcdir)/git.mk
