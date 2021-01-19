all:
	make -C src

man: xlibinput_calibrator.8


clean:
	rm -f xlibinput_calibrator.8
	rm -f xlibinput_calibrator.8.html
	make -C src clean

xlibinput_calibrator.8: xlibinput_calibrator.8.txt
	txt2man -s 8 -t xlibinput_calibrator -v 'General Commands Manual' $< > $@


