prefix = /usr/

all: src/xlibinput_calibrator

src/xlibinput_calibrator:
	make -C src

man: xlibinput_calibrator.8

xlibinput_calibrator.8: xlibinput_calibrator.8.txt
	txt2man -s 8 -t xlibinput_calibrator -v 'General Commands Manual' $< > $@

install: src/xlibinput_calibrator xlibinput_calibrator.8
	install -D src/xlibinput_calibrator \
	   $(DESTDIR)$(prefix)/bin/xlibinput_calibrator
	install -D xlibinput_calibrator.8 \
		$(DESTDIR)$(prefix)/share/man/man8/xlibinput_calibrator.8

distclean: clean

clean:
	rm -f xlibinput_calibrator.8
	rm -f xlibinput_calibrator.8.html
	make -C src clean

uninstall:
	-rm -f $(DESTDIR)$(prefix)/bin/xlibinput_calibrator
	-rm -f $(DESTDIR)$(prefix)/share/man/man8/xlibinput_calibrator.8

.PHONY: all install clean distclean uninstall

