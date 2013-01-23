#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.

PLUGIN = fritzbox
	
### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

# Use package data if installed...otherwise assume we're under the VDR source directory:
PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell pkg-config --variable=$(1) vdr || pkg-config --variable=$(1) ../../../vdr.pc))
LIBDIR = $(DESTDIR)$(call PKGCFG,libdir)
LOCDIR = $(DESTDIR)$(call PKGCFG,locdir)
TMPDIR = /tmp

### The compiler options:
export CFLAGS   = $(call PKGCFG,cflags)
export CXXFLAGS = $(call PKGCFG,cxxflags) -std=c++11
export LDFLAGS += -lgcrypt -lboost_system -lboost_thread -lpthread

### The version number of VDR's plugin API (taken from VDR's "config.h"):

APIVERSION = $(call PKGCFG,apiversion)
DOXYFILE = Doxyfile
DOXYGEN  = doxygen

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:
SOFILE = libvdr-$(PLUGIN).so
### The name of the static object file:
AFILE = libvdr-$(PLUGIN).a

### Includes and Defines (add further entries here):

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

### libfritz++
LIBFRITZ = libfritz++
INCLUDES += -I$(LIBFRITZ)
STATIC_LIBS = $(LIBFRITZ)/$(LIBFRITZ).a

### The object files (add further files here):

OBJS = $(PLUGIN).o fritzeventhandler.o log.o menu.o notifyosd.o setup.o	

### Internationalization (I18N):

PODIR     = po
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmo    = $(addsuffix .mo, $(foreach file, $(I18Npo), $(basename $(file))))
I18Nmsgs  = $(addprefix $(LOCDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

### Targets:
all: $(SOFILE) $(AFILE) i18n test $(LIBFRITZ)

$(SOFILE): $(OBJS) $(LIBFRITZ) 
	$(CXX) $(CXXFLAGS) -shared $(OBJS) $(LDFLAGS) $(STATIC_LIBS) -o $@
	
$(AFILE): $(OBJS) 
	ar ru libvdr-$(PLUGIN).a $(OBJS)

install-lib: $(SOFILE)
	install -D $^ $(LIBDIR)/$^.$(APIVERSION)

install: install-lib install-i18n

$(LIBFRITZ):
	@$(MAKE) -C $(LIBFRITZ)
	
%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz --exclude=.* --exclude=test --exclude=test.old --exclude=*.launch -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(OBJS) $(DEPFILE) *.so *.a *.tgz core* *~
	@-make -C $(LIBFRITZ) clean
	@-make -C test clean
	
%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c) $(wildcard libfritz++/*.cpp)
	xgettext -C -cTRANSLATORS --no-wrap -s --no-location -k -ktr -ktrNOOP -kI18N_NOOP \
	         --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) --msgid-bugs-address='<vdr@joachim-wilke.de>' -o $@ `ls $^`
	grep -v POT-Creation $(I18Npot) > $(I18Npot)~
	mv $(I18Npot)~ $(I18Npot)

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	install -D -m644 $< $@

i18n: $(I18Nmo) $(I18Npot)

install-i18n: $(I18Nmsgs)

test:
	test -d test && make -C test || true

srcdoc:
	@cp $(DOXYFILE) $(DOXYFILE).tmp
	@echo PROJECT_NUMBER = $(VERSION) >> $(DOXYFILE).tmp
	$(DOXYGEN) $(DOXYFILE).tmp
	@rm $(DOXYFILE).tmp

.PHONY: i18n test $(LIBFRITZ)
				
# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) $(CXXFLAGS) > $@

-include $(DEPFILE)
