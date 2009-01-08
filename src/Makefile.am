# plugindir is set in configure

##############################################################################
# change libgstplugin.la to something more suitable, e.g. libmysomething.la  #
##############################################################################
plugin_LTLIBRARIES = libsomatracker.la

##############################################################################
# for the next set of variables, rename the prefix if you renamed the .la,   #
#  e.g. libgstplugin_la_SOURCES => libmysomething_la_SOURCES                 #
#       libgstplugin_la_CFLAGS  => libmysomething_la_CFLAGS                  #
#       libgstplugin_la_LIBADD  => libmysomething_la_LIBADD                  #
#       libgstplugin_la_LDFLAGS => libmysomething_la_LDFLAGS                 #
##############################################################################

# sources used to compile this plug-in
libsomatracker_la_SOURCES = \
	gstsomacameraevent.c \
	gstsomapos2text.c \
	gstsomasynctracker.c \
	gstsomacmml2text.c \
	gstsomaposoverlay.c \
	gstsomatracker.c \
	utils.c

# flags used to compile this plugin
# add other _CFLAGS and _LIBS as needed
libsomatracker_la_CFLAGS = \
	$(GST_CFLAGS) \
	$(GST_BASE_CFLAGS) \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(SOMANETEVENT_CFLAGS) \
	$(CAIRO_CFLAGS)

libsomatracker_la_LIBADD = \
	$(GST_LIBS) \
	$(GST_PLUGINS_BASE_LIBS) \
	$(GST_BASE_LIBS) \
	$(SOMANETEVENT_LIBS) \
	$(CAIRO_LIBS)

libsomatracker_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libsomatracker_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = \
	gstsomacameraevent.h \
	gstsomapos2text.h \
	gstsomasynctracker.h \
	gstsomacmml2text.h \
	gstsomaposoverlay.h \
	utils.h
