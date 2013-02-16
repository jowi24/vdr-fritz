VDRDIR=../../vdr
PLUGIN_OBJECTS = $(patsubst %.cpp,%.o,$(wildcard ../*.cpp))
VDR_LIB = vdr.a
SI_LIB = $(VDRDIR)/libsi/libsi.a
GTEST_LIB = gtest/gtest.a
OBJS = $(patsubst %.cpp,%.o,$(wildcard *.cpp)) 
EXEC = fritzplugintest

VDR_LDFLAGS =  -ljpeg -lrt -ldl $(shell pkg-config --libs freetype2 fontconfig)  

all: $(EXEC)

$(EXEC): $(OBJS) $(PLUGIN_OBJECTS) $(GTEST_LIB) $(VDR_LIB) 
	$(CXX) $(CXXFLAGS) $(OBJS) $(PLUGIN_OBJECTS) $(GTEST_LIB) $(STATIC_LIBS) $(VDR_LIB) $(SI_LIB) $(LDFLAGS) $(VDR_LDFLAGS) -o $(EXEC)  

%.o: %.c
	$(CXX) $(CXXFLAGS) -o $@ -c $(DEFINES) $<
	
$(GTEST_LIB):
	make -C gtest
	
$(VDR_LIB):
	ar ru $(VDR_LIB) $(wildcard $(VDRDIR)/*.o) 
	
clean:
	@-make -C gtest clean
	@-rm $(OBJS) $(EXEC) $(VDR_LIB) 