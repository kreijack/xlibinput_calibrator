prefix = /usr/

all: src/xlibinput_calibrator

src/xlibinput_calibrator:
	make -C src

install: src/xlibinput_calibrator
	install -D src/xlibinput_calibrator \
	   $(DESTDIR)$(prefix)/bin/xlibinput_calibrator
	install -D xlibinput_calibrator.8 \
		$(DESTDIR)$(prefix)/share/man/man8/xlibinput_calibrator.8

clean:
	make -C src clean

distclean: clean

uninstall:
	-rm -f $(DESTDIR)$(prefix)/bin/xlibinput_calibrator

.PHONY: all install clean distclean uninstall
