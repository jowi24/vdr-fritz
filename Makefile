#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.

PLUGIN = fritzbox

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).cpp | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The directory environment:

# Use package data if installed...otherwise assume we're under the VDR source directory:
PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell pkg-config --variable=$(1) vdr || pkg-config --variable=$(1) ../../../vdr.pc))
LIBDIR = $(call PKGCFG,libdir)
LOCDIR = $(call PKGCFG,locdir)
PLGCFG = $(call PKGCFG,plgcfg)
#
TMPDIR ?= /tmp

### The compiler options:

export CFLAGS   = $(call PKGCFG,cflags)
export CXXFLAGS = $(call PKGCFG,cxxflags)

### The version number of VDR's plugin API:

APIVERSION = $(call PKGCFG,apiversion)

### Allow user defined options to overwrite defaults:

-include $(PLGCFG)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:

SOFILE = libvdr-$(PLUGIN).so

### Includes and Defines (add further entries here):

INCLUDES +=

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

### The object files (add further files here):

OBJS = $(patsubst %.cpp,%.o,$(wildcard *.cpp))

### Static libraries

LIBS = libfritz++/libfritz++.a libnet++/libnet++.a liblog++/liblog++.a libconv++/libconv++.a

STATIC_LIB_DIRS = $(dir $(LIBS))
STATIC_LIBS     = $(LIBS:%=$(CURDIR)/%)
CXXFLAGS       += -I$(CURDIR) -std=c++11
LDFLAGS        += -lboost_system -lboost_thread -lboost_regex -lpthread -lgcrypt
export STATIC_LIBS CXXFLAGS LDFLAGS

### Tests

TEST_DIRS       = $(wildcard $(addsuffix test,$(dir $(LIBS)))) $(wildcard test)

### Internationalization (I18N):

PODIR     = po
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmo    = $(addsuffix .mo, $(foreach file, $(I18Npo), $(basename $(file))))
I18Nmsgs  = $(addprefix $(DESTDIR)$(LOCDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

### Phony targets

.PHONY: all install i18n clean test $(STATIC_LIB_DIRS) $(TEST_DIRS)

### Targets:

all: $(SOFILE) i18n

$(SOFILE): $(OBJS) $(LIBS)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) $(LIBS) $(LDFLAGS) -o $@

%.a: $(STATIC_LIB_DIRS)
	@

$(STATIC_LIB_DIRS): 
	@$(MAKE) -C $@ $(@:/=).a

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

install-lib: $(SOFILE)
	install -D $^ $(DESTDIR)$(LIBDIR)/$^.$(APIVERSION)

install: install-lib install-i18n

test: $(STATIC_LIB_DIRS) $(TEST_DIRS)

$(TEST_DIRS): 
	@$(MAKE) -C $@

dist: $(I18Npo) clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@rm -rf $(TMPDIR)/$(ARCHIVE)/lib*/.git
	@rm -rf $(TMPDIR)/$(ARCHIVE)/test
	@rm -rf $(TMPDIR)/$(ARCHIVE)/lib*/test
	@tar czf $(PACKAGE).tgz --exclude=.* --exclude=test --exclude=test.old --exclude=*.launch -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@$(foreach LIB,$(STATIC_LIB_DIRS),$(MAKE) -C $(LIB) clean;)
	@$(foreach DIR,$(TEST_DIRS),$(MAKE) -C $(DIR) clean;)
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~

### I18n targets

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.cpp)
	mkdir -p $(PODIR)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) --msgid-bugs-address='<see README>' -o $@ `ls $^`

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(DESTDIR)$(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	install -D -m644 $< $@

i18n: $(I18Nmo) $(I18Npot)

install-i18n: $(I18Nmsgs)

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cpp) > $@

-include $(DEPFILE)
