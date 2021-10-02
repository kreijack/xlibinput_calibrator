/*
 * Copyright (c) 2009 Tias Guns
 * Copyright (c) 2020 Goffredo Baroncelli
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <X11/extensions/XInput.h>
#include <cassert>
#include <stdexcept>
#include <string>
#include <utility>

#include "xinput.hpp"
#include "mat9.hpp"

class WrongCalibratorException : public std::invalid_argument {
    public:
        WrongCalibratorException(const std::string& msg = "") :
            std::invalid_argument(msg) {}
};

class Calibrator
{
private:
    Display     *display = nullptr;
    //XDeviceInfo *devInfo;
    XID         device_id;
    Mat9        old_coeff;
    bool        reset_data;
    XInputTouch xinputtouch;
    std::string matrix_name;


public:
    Calibrator( std::string device_name_,
                    XID device_id,
                    const int thr_misclick,
                    const int thr_doubleclick,
                    std::string matrix_name_,
                    bool verbose);

    ~Calibrator();

    /// calculate and apply the calibration
    bool finish(int width, int height);


    bool set_calibration(const Mat9 &coeff);

    /// set the doubleclick treshold
    void set_threshold_doubleclick(int t)
    { threshold_doubleclick = t; }

    /// set the misclick treshold
    void set_threshold_misclick(int t)
    { threshold_misclick = t; }

    /// get the number of clicks already registered
    int get_numclicks() const
    { return clicked_x.size(); }

    /// reset clicks
    void reset()
    {  clicked_x.clear(); clicked_y.clear();}

    std::pair<int, int> get_point(int i) {
        return std::pair{clicked_x[i], clicked_y[i]};
    }

    /// add a click with the given coordinates
    bool add_click(int x, int y);

    bool save_calibration();
    bool output_xinput(const std::string &nf = "");
    bool output_xorgconfd(const std::string &nf = "");

    Mat9 get_coeff() { return result_coeff; }
    void set_identity();

private:

    /// check whether the coordinates are along the respective axis
    bool along_axis(int xy, int x0, int y0);

    std::vector<int> clicked_x, clicked_y;

    // Threshold to keep the same point from being clicked twice.
    // Set to zero if you don't want this check
    int threshold_doubleclick = 0;

    // Threshold to detect mis-clicks (clicks not along axes)
    // A lower value forces more precise calibration
    // Set to zero if you don't want this check
    int threshold_misclick = 0;

    std::string device_name;

    bool verbose = false;

    const int num_blocks = 8;

    Mat9 result_coeff;

    void setMatrix(const std::string &name, const Mat9 &coeff);
    void getMatrix(const std::string &name, Mat9 &coeff);
};
