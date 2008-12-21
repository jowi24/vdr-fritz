#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
# IPORTANT: the presence of this macro is important for the Make.config
# file. So it must be defined, even if it is not used here!
#
PLUGIN = fritzbox
	
### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The C++ compiler and options:

CXX      ?= g++
CXXFLAGS ?= -fPIC -g -O2 -Wall -Woverloaded-virtual

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

### The version number of VDR's plugin API (taken from VDR's "config.h"):

APIVERSION = $(shell sed -ne '/define APIVERSION/s/^.*"\(.*\)".*$$/\1/p' $(VDRDIR)/config.h)
VDRVERSNUM = $(shell grep 'define VDRVERSNUM ' $(VDRDIR)/config.h | awk '{ print $$3 }' | sed -e 's/"//g')
DOXYFILE = Doxyfile
DOXYGEN  = doxygen

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### Includes and Defines (add further entries here):

INCLUDES += -I$(VDRDIR)/include

DEFINES += -D_GNU_SOURCE -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

### libfritz++
LIBFRITZ = libfritz++
INCLUDES += -I$(LIBFRITZ)
LIBS += $(LIBFRITZ)/$(LIBFRITZ).a

### libpthread++
LIBPTHREAD = libpthread++
INCLUDES += -I$(LIBPTHREAD)
LIBS += $(LIBPTHREAD)/$(LIBPTHREAD).a

### libtcpclient++
LIBTCPCLIENT = libtcpclient++
INCLUDES += -I$(LIBTCPCLIENT)
LIBS += $(LIBTCPCLIENT)/$(LIBTCPCLIENT).a 


### The object files (add further files here):

OBJS = $(PLUGIN).o fritzeventhandler.o log.o menu.o notifyosd.o setup.o	  

### Targets:
all: libvdr-$(PLUGIN).so i18n
	@-cp --remove-destination libvdr-$(PLUGIN).so $(LIBDIR)/libvdr-$(PLUGIN).so.$(APIVERSION) 
## TODO: Prevent so file from beeing build every time
libvdr-$(PLUGIN).so: $(OBJS) libfritz 
	$(CXX) $(CXXFLAGS) -shared $(OBJS) $(LIBS) -o $@
	
libfritz: libtcpclient libpthread
	$(MAKE) -C $(LIBFRITZ)	
	
libtcpclient:
	$(MAKE) -C $(LIBTCPCLIENT)
	
libpthread:
	$(MAKE) -C $(LIBPTHREAD)

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@find $(TMPDIR)/$(ARCHIVE) -type d | grep -e \.svn$ | xargs -i rm -rf {}
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~
	@-make -C $(LIBFRITZ) clean
	@-make -C $(LIBTCPCLIENT) clean
	@-make -C $(LIBPTHREAD) clean

### Internationalization (I18N):

PODIR     = po
LOCALEDIR = $(VDRDIR)/locale
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmsgs  = $(addprefix $(LOCALEDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c)
	xgettext -C -cTRANSLATORS --no-wrap -F -k -ktr -ktrNOOP --msgid-bugs-address='<vdr@joachim-wilke.de>' -o $@ $^

%.po: $(I18Npot)
	msgmerge -U --no-wrap -F --backup=none -q $@ $<
	@touch $@

$(I18Nmsgs): $(LOCALEDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	@mkdir -p $(dir $@)
	cp $< $@

.PHONY: i18n
i18n: $(I18Nmsgs)

srcdoc:
	@cp $(DOXYFILE) $(DOXYFILE).tmp
	@echo PROJECT_NUMBER = $(VERSION) >> $(DOXYFILE).tmp
	$(DOXYGEN) $(DOXYFILE).tmp
	@rm $(DOXYFILE).tmp
				
# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)
