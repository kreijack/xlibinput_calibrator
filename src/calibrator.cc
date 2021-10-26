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

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <ctype.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cassert>

#include "calibrator.hpp"

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 1
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 0
#endif


/// Names of the points
enum {
    UL = 0, // Upper-left
    UR = 1, // Upper-right
    LL = 2, // Lower-left
    LR = 3,  // Lower-right
    NUM_POINTS
};

void Calibrator::getMatrix(const std::string &name, Mat9 &coeff) {

    std::vector<std::string> values;
    auto ret = xinputtouch.get_prop(device_id, name.c_str(), values);

    if (ret < 0 || values.size() != 9)
        throw WrongCalibratorException("Libinput: \"libinput Calibration Matrix\" property missing, not a (valid) libinput device");

    mat9_set_identity(coeff);
    for (unsigned int i = 0 ; i < 9 ; i++)
        coeff[i] = std::stod(values[i]);

}

void Calibrator::setMatrix(const std::string &name, const Mat9 &coeff) {

    Atom float_atom = XInternAtom(display, "FLOAT", false);
    int format = 32;
    std::vector<std::string> values;

    for (auto x : coeff.coeff)
        values.push_back(std::to_string(x));

    auto ret = xinputtouch.set_prop(device_id, name.c_str(), float_atom, format, values);
    if (ret < 0)
        throw WrongCalibratorException("Libinput: \"libinput Calibration Matrix\" property missing, not a (valid) libinput device");


}

// Constructor
Calibrator::Calibrator(std::string device_name_,
                                 XID device_id_,
                                 const int thr_misclick_,
                                 const int thr_doubleclick_,
                                 std::string matrix_name_,
                                 bool verbose_) :
        threshold_doubleclick(thr_doubleclick_),
        threshold_misclick(thr_misclick_),
        device_name(device_name_),
        verbose(verbose_)
{
    device_id = device_id_;
    matrix_name = matrix_name_;

    // init
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        throw WrongCalibratorException("Libinput: Unable to connect to X server");
    }

    getMatrix(matrix_name, old_coeff);
    reset_data = true;
}

void Calibrator::set_identity()
{
    Mat9 coeff;
    mat9_set_identity(coeff);
    setMatrix(matrix_name, coeff);
}

bool Calibrator::finish(int width, int height)
{

    if (verbose) {
        printf("Calibrating Libinput driver:\n");
        printf("\tDevice:%s\n", device_name.c_str());
        printf("\tDevice-ID:%lu\n", device_id);
    }

    if (get_numclicks() != NUM_POINTS) {
        return false;
    }

    /*
     * Assuming that
     *
     *  [a  b  c]     [tx_i]     [sx_i]
     *  [d  e  f]  x  [ty_i]  =  [sy_i]
     *  [0  0  1]     [  1 ]     [ 1  ]
     *
     *      ^          ^        ^
     *      C          Ti       Si
     *
     *  Where:
     *   - a,b ...f      -> conversion matrix
     *   - tx_i, ty_i    -> 'i'th touch x,y
     *   - sx_i, sy_i    -> 'i'th screen x,y
     *  this means:
     *
     *            ⎡tx_1  tx_2  tx_3⎤     ⎡sx_1  sx_2  sx_3⎤
     *            ⎢                ⎥     ⎢                ⎥
     *        C x ⎢ty_1  ty_2  ty_3⎥  =  ⎢sy_1  sy_2  sy_3⎥
     *            ⎢                ⎥     ⎢                ⎥
     *            ⎣  1     1     1 ⎦     ⎣  1     1     1 ⎦
     *
     *            ⎡sx_1  sx_2  sx_3⎤     ⎡tx_1  tx_2  tx_3⎤ ^ -1
     *            ⎢                ⎥     ⎢                ⎥
     *        C = ⎢sy_1  sy_2  sy_3⎥  x  ⎢ty_1  ty_2  ty_3⎥
     *            ⎢                ⎥     ⎢                ⎥
     *            ⎣  1     1     1 ⎦     ⎣  1     1     1 ⎦
     *
     */

    Mat9 coeff_tmp, tmi, tm, ts, coeff;

    const float xl = width /  (float)num_blocks;
    const float xr = width /  (float)num_blocks * (num_blocks - 1);
    const float yu = height / (float)num_blocks;
    const float yl = height / (float)num_blocks * (num_blocks - 1);

    /* skip LR */
    tm.set(clicked_x[UL],   clicked_x[UR],  clicked_x[LL],
           clicked_y[UL],   clicked_y[UR],  clicked_y[LL],
           1,               1,              1);
    ts.set(xl,              xr,             xl,
           yu,              yu,             yl,
           1,               1,              1);

    mat9_invert(tm, tmi);
    mat9_product(ts, tmi, coeff);

    /* skip UL */
    tm.set(clicked_x[LR],   clicked_x[UR],  clicked_x[LL],
           clicked_y[LR],   clicked_y[UR],  clicked_y[LL],
           1,               1,              1);
    ts.set(xr,              xr,             xl,
           yl,              yu,             yl,
           1,               1,              1);

    mat9_invert(tm, tmi);
    mat9_product(ts, tmi, coeff_tmp);
    mat9_sum(coeff_tmp, coeff);

    /* skip UR */
    tm.set(clicked_x[LR],   clicked_x[UL],  clicked_x[LL],
           clicked_y[LR],   clicked_y[UL],  clicked_y[LL],
           1,               1,              1);
    ts.set(xr,              xl,             xl,
           yl,              yu,             yl,
           1,               1,              1);

    mat9_invert(tm, tmi);
    mat9_product(ts, tmi, coeff_tmp);
    mat9_sum(coeff_tmp, coeff);

    /* skip LL */
    tm.set(clicked_x[LR],   clicked_x[UL],  clicked_x[UR],
           clicked_y[LR],   clicked_y[UL],  clicked_y[UR],
           1,               1,              1);
    ts.set(xr,              xl,             xr,
           yl,              yu,             yu,
           1,               1,              1);

    mat9_invert(tm, tmi);
    mat9_product(ts, tmi, coeff_tmp);
    mat9_sum(coeff_tmp, coeff);

    /*
     * the final matrix is the average of the previous computed ones
     */
    mat9_product(1.0/4.0, coeff);

    /*
     *             Coefficient normalization
     *
     * The matrix to pass to libinput has to be normalized; we need to
     * translate and scale the coeffiecient so the matrix can operate in
     * a space where the coordinates x and y (both in input and output) are
     * in the range 0..1
     *
     * To do that, assume:
     *
     * a "translation" matrix is
     *       [ 1 0 dx ]
     * Tr =  [ 0 1 dy ]
     *       [ 0 0 1  ]
     *
     * a "scale" matrix is
     *       [ sx 0  0 ]
     * Sc =  [ 0  sy 0 ]
     *       [ 0  0  1 ]
     *
     * To change the coordinate from the normalizate space to the screen space
     * - First we need to scale from (0..1 x 0..1) to (width x height); so
     *   sx = maxx - minx + 1 = width, sy = maxy - miny + 1 = height
     * - Second we need to translate
     *   from (0..width-1 x 0..hight-1) to (minx..maxx x miny..maxy)
     *   so dx = minx, dy = miny
     *
     * So
     *    C = Tr x Sc x Cn x Sc^-1 x Tc^-1
     * this means that
     *    Cn = Sc^-1 x Tr^-1 x C x Tr x Sc
     * where
     *      C is the Calibration matrix in the "screen" spaces
     *      Cn is the normalizated matrix that can be passed to libinput
     *
     * Because in the screen space usually minx=miny=0, this means
     * that dx == dy == 0 -> T == T^-1 == identity. So we can write
     *      Cn = Sc^-1 x C x Sc
     *
     *
     * and because
     *
     *                ⎡a  b  c⎤
     *                ⎢       ⎥
     *        C   =   ⎢d  e  f⎥
     *                ⎢       ⎥
     *                ⎣0  0  1⎦
     *
     * then
     *              ⎡      b⋅sy  c ⎤
     *              ⎢ a    ────  ──⎥
     *              ⎢       sx   sx⎥
     *              ⎢              ⎥
     *       Cn =   ⎢d⋅sx        f ⎥
     *              ⎢────   e    ──⎥
     *              ⎢ sy         sy⎥
     *              ⎢              ⎥
     *              ⎣ 0     0    1 ⎦
     *
     *
     *
     * See libinput function evdev_device_calibrate() (in src/evdev.c)
     *
     * As further reference, if dx/dy are not zero:
     *
     *              ⎡        b⋅sy            b⋅dy⋅sy   c      ⎤
     *              ⎢ a      ────     a⋅dx + ─────── + ── - dx⎥
     *              ⎢         sx                sx     sx     ⎥
     *              ⎢                                         ⎥
     *              ⎢d⋅sx             d⋅dx⋅sx               f ⎥
     *       Cn =   ⎢────     e       ─────── + dy⋅e - dy + ──⎥
     *              ⎢ sy                 sy                 sy⎥
     *              ⎢                                         ⎥
     *              ⎣ 0       0                  1            ⎦
     */

    coeff[1] *= (float)height/width;
    coeff[2] *= 1.0/width;

    coeff[3] *= (float)width/height;
    coeff[5] *= 1.0/height;

    /*
     * Sometimes, the last row values are like -0.0, -0.0, 1
     * update to the right values, otherwise libinput complaints !
     */
    coeff[6] = 0.0;
    coeff[7] = 0.0;
    coeff[8] = 1.0;

    result_coeff = coeff;

    return true;
}

// Activate calibrated data and output it
bool Calibrator::save_calibration()
{
    auto success = set_calibration(result_coeff);

    // close
    XSync(display, False);
    reset_data = false;

    return success;
}

bool Calibrator::set_calibration(const Mat9 &coeff) {
    try {
        setMatrix(matrix_name, coeff);
    } catch(...) {
        if (verbose)
            printf("Failed to apply axis calibration.\n");
        return false;;
    }

    if (verbose)
            printf("Successfully applied axis calibration.\n");

    return true;
}

bool Calibrator::add_click(int x, int y)
{
    // Double-click detection
    if (threshold_doubleclick > 0 && get_numclicks() > 0) {
        int i = get_numclicks() - 1;
        while (i >= 0) {
            if (abs(x - clicked_x[i]) <= threshold_doubleclick
                && abs(y - clicked_y[i]) <= threshold_doubleclick) {
                if (verbose) {
                    printf("WARNING: Not adding click %i (X=%i, Y=%i): within %i pixels of previous click\n",
                         get_numclicks(), x, y, threshold_doubleclick);
                }
                return false;
            }
            i--;
        }
    }

    // Mis-click detection
    if (threshold_misclick > 0 && get_numclicks() > 0) {
        bool misclick = true;

        switch (get_numclicks()) {
            case 1:
                // check that along one axis of first point
                if (along_axis(x,clicked_x[UL],clicked_y[UL]) ||
                        along_axis(y,clicked_x[UL],clicked_y[UL]))
                {
                    misclick = false;
                } else if (verbose) {
                    printf("WARNING: Mis-click detected, click %i (X=%i, Y=%i) not aligned with click 0 (X=%i, Y=%i) (threshold=%i)\n",
                            get_numclicks(), x, y, clicked_x[UL], clicked_y[UL], threshold_misclick);
                }
                break;

            case 2:
                // check that along other axis of first point than second point
                if ((along_axis( y, clicked_x[UL], clicked_y[UL])
                            && along_axis( clicked_x[UR], clicked_x[UL], clicked_y[UL]))
                        || (along_axis( x, clicked_x[UL], clicked_y[UL])
                            && along_axis( clicked_y[UR], clicked_x[UL], clicked_y[UL])))
                {
                    misclick = false;
                } else if (verbose) {
                    printf("WARNING: Mis-click detected, click %i (X=%i, Y=%i) not aligned with click 0 (X=%i, Y=%i) or click 1 (X=%i, Y=%i) (threshold=%i)\n",
                            get_numclicks(), x, y, clicked_x[UL], clicked_y[UL], clicked_x[UR], clicked_y[UR], threshold_misclick);
                }
                break;

            case 3:
                // check that along both axis of second and third point
                if ( ( along_axis( x, clicked_x[UR], clicked_y[UR])
                            &&   along_axis( y, clicked_x[LL], clicked_y[LL]) )
                        ||( along_axis( y, clicked_x[UR], clicked_y[UR])
                            &&  along_axis( x, clicked_x[LL], clicked_y[LL]) ) )
                {
                    misclick = false;
                } else if (verbose) {
                    printf("WARNING: Mis-click detected, click %i (X=%i, Y=%i) not aligned with click 1 (X=%i, Y=%i) or click 2 (X=%i, Y=%i) (threshold=%i)\n",
                            get_numclicks(), x, y, clicked_x[UR], clicked_y[UR], clicked_x[LL], clicked_y[LL], threshold_misclick);
                }
        }

        if (misclick) {
            reset();
            return false;
        }
    }

    clicked_x.push_back(x);
    clicked_y.push_back(y);

    return true;
}

inline bool Calibrator::along_axis(int xy, int x0, int y0)
{
    return ((abs(xy - x0) <= threshold_misclick) ||
            (abs(xy - y0) <= threshold_misclick));
}

bool Calibrator::output_xinput(const std::string &output_filename)
{

    std::string devname;
    if (device_name.size() == 0)
        devname = std::to_string(device_id);
    else
        devname = device_name;

    if(output_filename == "")
        printf("Install the 'xinput' tool and copy the command(s) below in a script that starts with your X session\n");
    else
        printf("Writing calibration script to '%s'\n", output_filename.c_str());

    // create startup script
    char line[2000];
    std::string outstr;

    snprintf(line, sizeof(line)-1,
                "\n       xinput set-float-prop \"%s\" \"%s"
                "\" \\\n            %f %f %f %f %f \\\n            "
                "%f %f %f %f\n\n",
                devname.c_str(), matrix_name.c_str(),
                result_coeff[0], result_coeff[1], result_coeff[2],
                result_coeff[3], result_coeff[4], result_coeff[5],
                result_coeff[6], result_coeff[7], result_coeff[8]
                           );
    outstr += line;

    // console out
    printf("%s", outstr.c_str());
    // file out
    if(output_filename != "") {
        FILE* fid = fopen(output_filename.c_str(), "w");
        if (fid == NULL) {
            fprintf(stderr, "Error: Can't open '%s' for writing. Make sure you have the necessary rights\n", output_filename.c_str());
            fprintf(stderr, "New calibration data NOT saved\n");
            return false;
        }
        fprintf(fid, "%s", outstr.c_str());
        fclose(fid);
    }

    return true;
}

bool Calibrator::output_xorgconfd(const std::string &output_filename)
{

    if(output_filename.size() == 0)
        printf("Copy the snippet below into '/etc/X11/xorg.conf.d/99-calibration.conf' (/usr/share/X11/xorg.conf.d/ in some distro's)\n");
    else
        printf("Writing xorg.conf calibration data to '%s'\n", output_filename.c_str());

    // xorg.conf.d snippet
    char line[2000];
    std::string outstr;
    std::string devname;
    if (device_name.size() == 0)
        devname = std::to_string(device_id);
    else
        devname = device_name;

    outstr += "\n";
    outstr += "Section \"InputClass\"\n";
    outstr += "\tIdentifier\t\"calibration\"\n";
    sprintf(line, "\tMatchProduct\t\"%s\"\n", devname.c_str());
    outstr += line;
    snprintf(line, sizeof(line) - 1,
                "\tOption\t\t\"CalibrationMatrix\"\t\"%f %f %f %f %f %f %f %f %f \"\n",
                result_coeff[0], result_coeff[1], result_coeff[2], result_coeff[3], result_coeff[4], result_coeff[5],
                result_coeff[6], result_coeff[7], result_coeff[8]);
    outstr += line;
    outstr += "EndSection\n\n";

    // console out
    printf("%s", outstr.c_str());

    // file out
    if(output_filename.size()) {
        FILE* fid = fopen(output_filename.c_str(), "w");
        if (fid == NULL) {
            fprintf(stderr, "Error: Can't open '%s' for writing. Make sure you have the necessary rights\n", output_filename.c_str());
            fprintf(stderr, "New calibration data NOT saved\n");
            return false;
        }
        fprintf(fid, "%s", outstr.c_str());
        fclose(fid);
    }

    return true;
}

Calibrator::~Calibrator() {
    if (reset_data) {
        printf("Restore previous calibration values\n");
        set_calibration(old_coeff);
    }
    if (verbose) {
        Mat9 coeff;
        getMatrix(matrix_name, coeff);
        printf("Current calibration values (from XInput):\n");
        mat9_print(coeff);
    }
    if (display)
        XCloseDisplay(display);
}
