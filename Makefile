prefix = /usr/
locales := tr
all: build

build: src/xlibinput_calibrator locale

src/xlibinput_calibrator:
	make -C src

man: xlibinput_calibrator.8

locale:
	@for lang in $(locales) ; do \
	    echo Building locale: $$lang ; \
	    msgfmt --output-file=po/$$lang.mo po/$$lang.po ;\
	done

xlibinput_calibrator.8: xlibinput_calibrator.8.txt
	txt2man -s 8 -t xlibinput_calibrator -v 'General Commands Manual' $< > $@

install: src/xlibinput_calibrator xlibinput_calibrator.8
	install -D src/xlibinput_calibrator \
	   $(DESTDIR)$(prefix)/bin/xlibinput_calibrator
	install -D xlibinput_calibrator.8 \
		$(DESTDIR)$(prefix)/share/man/man8/xlibinput_calibrator.8
	@for lang in $(locales) ; do \
	    echo Installing locale: $$lang ; \
	    install -D po/$$lang.mo $(DESTDIR)$(prefix)/share/locale/$$lang/LC_MESSAGES/xlibinput-calibrator.mo ; \
	done

distclean: clean

clean:
	rm -f xlibinput_calibrator.8
	rm -f xlibinput_calibrator.8.html
	rm -f po/*.mo
	make -C src clean

uninstall:
	-rm -f $(DESTDIR)$(prefix)/bin/xlibinput_calibrator
	-rm -f $(DESTDIR)$(prefix)/share/man/man8/xlibinput_calibrator.8


pot:
	xgettext --keyword=_ --language=C --add-comments --sort-output -o po/xlibinput-calibrator.pot src/*.cc
	@for lang in $(locales) ; do \
	    echo Updating locale: $$lang ; \
	    msgmerge --update po/$$lang po/xlibinput-calibrator.pot ; \
	done

.PHONY: all install clean distclean uninstall

