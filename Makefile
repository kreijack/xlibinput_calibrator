CXXFLAGS=-Wall -pedantic -std=c++17
SRCS=main.cpp gui_x11.cpp version.cc xinput.cc mat9.cc transform.cc
OBJECTS= $(SRCS:.cc=.o)
LIBS=-lX11 -lXi
LDFLAGS=-std=c++17

all: xlibinput_calibrator

clean:
	rm -f *.o
	rm -f debug-*
	rm -f xlibinput_calibrator

debug-xinput: xinput.cc xinput.hpp
	$(CXX) $(CXXFLAGS) $(LIBS) -o debug-xinput -DDEBUG xinput.cc

version.cc: .git/HEAD .git/index
	echo "const char *gitversion = \"$(shell git describe --abbrev=4 --dirty --always --tags)\";" > $@


xlibinput_calibrator: $(OBJECTS)
	$(CXX) $(LDFLAGS) -o xlibinput_calibrator $^ $(LIBS)

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
