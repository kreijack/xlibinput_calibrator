# xlibinput_calibrator

## Introduction

The aim of this project is to allow to calibrate a touch screen when X11 relies on libinput.
For older X11, which doesn't relies on libinput, you have to use the old xinput_calibrator.

This project is derived from xinput_calibrator.

## Usage
```
xlibinput-calibrator [opts]
  --output-file-x11-config=<filename>   save the output to filename
  --output-file-xinput-cmd=<filename>   save the output to filename
  --threshold-misclick=<nn>     set the threshold for misclick to <nn>
  --threshold-doubleclick=<nn>  set the threshold for doubleckick to <nn>
  --device-name=<devname>       set the touch screen device by name
  --device-id=<devid>           set the touch screen device by id
  --matrix-name=<matrix name>   set the calibration matrix name
  --show-x11-config             show the config for X11
  --show-xinput-cmd             show the config for libinput
  --show-matrix                 show the final matrix
  --verbose                     set verbose to on
  --dont-save                   don't update X11 setting
  --matrix=x1,x2..x9            start coefficent matrix
  --monitor-nr=<n>              show the ouput in the monitor '<n>'
  
xlibinput-calibrator --list-devices
```

The possible outcomes of this command are the following:

* Set the new configuration in X11 (default); you can prevent this using the switch *--dont-save*.
* Show the X11 configuration of the new configuration matrix (*--show-x11-config*); optionally
you can save the setting in a file (*--output-file-x11-config=*).
* Show the xinput command for the new configuration matrix (*--show-xinput-cmd*); optionally
you can save the command in a file (*--output-file-xinput-cmd=*).

**xlibinput_calibrator** selects automatically the device to operate. If for some reason this is not possible (avaliability of multiple devices), it is possible to select explicitly the device with *--device-id=* or *--device-name=* options (the former takes precedence).

*--threshold-misclick=* set the threshold for accept or reject a click. The four clicks have to be the angles of a rectagle. The value passed are the maximum  allowable distance between the click and the ideal point. If the value is 0 (default), the check is not performed.

*--threshold-douleclick=* set the threshold for accept or reject a click. It sets the minimum distance between clicks to accept them. If the value is 0 (default), the check is not performed.

*--matrix=* sets the intial matrix before doing the calibration. By default **xlibinput_calibrator**
sets the calibration matrix to the identity (i.e. all 1 in the diagonal). With this option it is possible to set another matrix. Note that if something goes wrong or the calibration fails, the original matrix is set in X11.

## Dependencies
* libxi-dev
* libx11-dev
* C++ 17 compiler
* xrandr (optional)
* txt2man

## Compile

Go under **src/**, and run **make**:

	$ git clone https://github.com/kreijack/xlibinput_calibrator.git
	Cloning into 'xlibinput_calibrator'...
	[...]
	$ cd xlibinput_calibrator/src/
	xlibinput_calibrator/src$ make
	g++ -MT main.o -MMD -MP -MF .d/main.Td -Wall -pedantic -std=c++17   -c -o main.o main.cc
	[..]
	xlibinput_calibrator/src$ ls -l xlibinput_calibrator
	-rwxr-xr-x 1 ghigo ghigo 208416 Jan 17 19:58 xlibinput_calibrator


## Man page

To generate the man page, run "make man" in the root folder:

    $ make man
    txt2man -s 8 -t xlibinput_calibrator -v 'General Commands Manual' xlibinput_calibrator.8.txt > xlibinput_calibrator.8


## AUTHORS:
This project is derived from xinput_calibrator (mainly gui_x11).
Some pieces derived from the xorg-xinput sources.
The rest of the code is by Goffredo Baroncelli <kreijack@inwind.it>

TODO:
- [ ] Bugfix
- [X] Add manpage
- [ ] Add debian package (debian/....)
- [ ] Add rpm package (spec file)
