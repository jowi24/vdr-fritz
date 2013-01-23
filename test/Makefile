VDRDIR=../../vdr
PLUGIN_LIB = ../libvdr-fritzbox.a 
FRITZ_LIB  = ../libfritz++/libfritz++.a
VDR_LIB = vdr.a
SI_LIB = $(VDRDIR)/libsi/libsi.a
GTEST_LIB = gtest/gtest.a
OBJS = $(patsubst %.c,%.o,$(wildcard *.c)) 
EXEC = fritzplugintest

INCLUDES += -I$(VDRDIR)/include
INCLUDES += -I.. -I../libfritz++ 
LDFLAGS += -ljpeg -lrt -ldl $(shell pkg-config --libs freetype2 fontconfig) $(GTEST_LIB) $(PLUGIN_LIB) $(FRITZ_LIB) $(VDR_LIB) $(SI_LIB) 

all: $(EXEC)

$(EXEC): $(OBJS) $(PLUGIN_LIB) $(GTEST_LIB) $(VDR_LIB) 
	$(CXX) $(CXXFLAGS) -o $(EXEC) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CXX) $(CXXFLAGS) -o $@ -c $(DEFINES) $(INCLUDES) $<
	
$(GTEST_LIB):
	make -C gtest
	
$(PLUGIN_LIB):
	make -C ..

$(VDR_LIB):
	ar ru $(VDR_LIB) $(wildcard $(VDRDIR)/*.o) 
	
clean:
	@-make -C gtest clean
	@-rm $(OBJS) $(EXEC) $(VDR_LIB) 