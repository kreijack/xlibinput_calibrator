all:
	make -C src

clean:
	rm -f xlibinput_calibrator.8
	rm -f xlibinput_calibrator.8.html
	make -C src clean

xlibinput_calibrator.8: xlibinput_calibrator.8.ronn
	ronn $<


