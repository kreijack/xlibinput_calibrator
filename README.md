# xlibinput_calibrator

## Introduction

This project is derived from xinput_calibrator. Even tough a lot of changes where performed the GUI derives it.

The aim of this project is to allow to calibrate a touch when X11 relies on libinput.
For older X11 libraries, you have to use the
old xinput_calibrator.

## Usage
```
xlibinput_calibrator [opts]
--output-file=<filename>      save the output to filename
--threshold-misclick=<nn>     set the threshold for misclick to <nn>
--threshold-doubleclick=<nn>  set the threshold for doubleckick to <nn>
--device-name=<devname>       set the touch screen device by name
--device-id=<devid>           set the touch screen device by id
--show-x11-config             show the config for X11
--show-xinput-config          show the config for libinput
--show-matrix                 show the final matrix
--verbose                     set verbose to on
--not-save                    don't update X11 setting
--matrix=x1,x2..x9            start coefficent matrix
```

## Dependencies
* libxi-dev
* libx11-dev
* C++ 17 compiler
