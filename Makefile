
LIBS += -linput -ludev

clean:
	rm *.o
	rm debug-*

debug-libinput-list-devices:libinput-list-devices.cc libinput-list-devices.hpp
	$(CXX) $(CXXFLAGS) $(LIBS) -DDEBUG -o debug-libinput-list-devices \
		libinput-list-devices.cc

