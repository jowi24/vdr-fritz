LIB = gtest.a
OBJS = gtest_main.o gtest-all.o

INCLUDES += -I..

all: $(OBJS) $(LIB)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -o $@ -c $(DEFINES) $(INCLUDES) $<
	
$(LIB): $(OBJS)
	ar ru $(LIB) $(OBJS)
	@-echo Built $(LIB).
	
clean:
	@-rm $(OBJS) $(LIB)