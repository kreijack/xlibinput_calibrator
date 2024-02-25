CXXFLAGS=-Wall -pedantic -std=c++17
SRCS=main.cc gui_x11.cc version.cc xinput.cc mat9.cc calibrator.cc
OBJECTS= $(SRCS:.cc=.o)
LIBS=-lX11 -lXi -lXrandr
LDFLAGS=-std=c++17

all: xlibinput_calibrator

clean:
	rm -f *.o
	rm -f debug-*
	rm -f xlibinput_calibrator
	rm -rf .d
	rm -f version.cc
	rm -f test_mat9

../.git/HEAD:

../.git/index:

version.cc: ../.git/HEAD ../.git/index
	echo "const char *gitversion = \"$(shell git describe --abbrev=4 --dirty --always --tags 2>/dev/null || echo '<undef>' )\";" > $@

xlibinput_calibrator: $(OBJECTS)
	$(CXX) $(LDFLAGS) -o xlibinput_calibrator $^ $(LIBS)

test_mat9: mat9.cc mat9.hpp
	$(CXX) $(LDFLAGS) -DTEST_MAT9 -o test_mat9 mat9.cc
	./test_mat9

# -----------------------------------

DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

%.o : %.c
%.o : %.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

%.o : %.cc
%.o : %.cc $(DEPDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

%.o : %.cxx
%.o : %.cxx $(DEPDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

-include $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS)))
