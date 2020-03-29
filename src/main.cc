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

#include <unistd.h>
#include <cstdio>

#include "gui_x11.hpp"
#include "calibrator.hpp"

extern const char *gitversion;

void show_help() {
    printf("usage: xlibinput_calibrator [opts]\n"
        "--output-file-x11-config=<filename>   save the output to filename\n"
        "--output-file-xinput-cmd=<filename>   save the output to filename\n"
        "--threshold-misclick=<nn>     set the threshold for misclick to <nn>\n"
        "--threshold-doubleclick=<nn>  set the threshold for doubleckick to <nn>\n"
        "--device-name=<devname>       set the touch screen device by name\n"
        "--device-id=<devid>           set the touch screen device by id\n"
        "--show-x11-config             show the config for X11\n"
        "--show-xinput-cmd             show the config for libinput\n"
        "--show-matrix                 show the final matrix\n"
        "--verbose                     set verbose to on\n"
        "--not-save                    don't update X11 setting\n"
        "--matrix=x1,x2..x9            start coefficent matrix\n"
        "\n"
        "version: %s\n"
        "\n",
        gitversion
    );
}

bool starts_with(std::string_view s1, std::string_view s2)
{
    if (s1.size() < s2.size())
        return false;
    return !s1.compare(0, s2.size(), s2, 0, s2.size());
}

int stoi(std::string_view s)
{
    int ret = 0;
    for (auto c : s)
        ret = ret * 10 + c-'0';
    return ret;
}

unsigned long stou(std::string_view s)
{
    unsigned long ret = 0;
    for (auto c : s)
        ret = ret * 10 + c-'0';
    return ret;
}

int main(int argc, char** argv)
{

    std::string output_file_x11;
    std::string output_file_xinput;
    bool verbose = false;
    int thr_misclick = 0;
    int thr_doubleclick = 0;
    std::string device_name;
    XID device_id = (XID)-1;
    bool show_matrix = false;
    bool show_conf_x11 = false;
    bool show_conf_xinput = false;
    bool not_save = false;
    std::string start_coeff;

    for (int i = 1 ; i < argc ; i++) {
        const std::string_view arg{argv[i]};

        if (starts_with(arg, "--output-file-x11-config=")) {
            output_file_x11 = arg.substr(25);
        } else if (starts_with(arg, "--output-file-xinput-cmd=")) {
            output_file_xinput = arg.substr(25);
        } else if (starts_with(arg, "--threshold-misclick=")) {
            thr_misclick = stoi(arg.substr(21));
        } else if (starts_with(arg, "--threshold-doubleclick=")) {
            thr_doubleclick = stoi(arg.substr(24));
        } else if (starts_with(arg, "--device-name=")) {
            device_name = std::string(arg.substr(14));
        } else if (starts_with(arg, "--device-id=")) {
            device_id = stou(arg.substr(12));
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--not-save") {
            not_save = true;
        } else if (arg == "--show-x11-config") {
            show_conf_x11 = true;
        } else if (arg == "--show-xinput-cmd") {
            show_conf_xinput = true;
        } else if (arg == "--show-matrix") {
            show_matrix = true;
        } else if (starts_with(arg, "--start-matrix=")) {
            start_coeff = arg.substr(15);
        } else if (arg == "--help" || arg == "-h") {
            show_help();
            exit(0);
        } else {
            printf("ERROR: unknown parameter '%s'\n", argv[i]);
            show_help();
            exit(1);
        }
    }

    if (verbose) {
        printf("show-matrix:                %s\n", show_matrix ? "yes" : "no");
        printf("show-x11-config:            %s\n", show_conf_x11 ? "yes" : "no");
        printf("show-libinput-config:       %s\n", show_conf_xinput ? "yes" : "no");
        printf("not-save:                   %s\n", show_conf_xinput ? "yes" : "no");
        if (device_id != (XID)-1)
            printf("device-id:                  %lu\n", device_id);
        else
            printf("device-id:                  <UNSET>\n");
        printf("device-name:                '%s'\n", device_name.c_str());
        printf("output-file-x11-config:     '%s'\n", output_file_x11.c_str());
        printf("output-file-xinput-config:  '%s'\n", output_file_xinput.c_str());
        printf("threshold-misclick:         %d\n", thr_misclick);
        printf("threshold-doubleclick:      %d\n", thr_doubleclick);
    }


    GuiCalibratorX11 gui;
    Calibrator  calib(device_name, device_id, thr_misclick, thr_doubleclick,
                        verbose);

    if (start_coeff.size() == 0) {
        calib.set_identity();
    } else {
        Mat9 coeff;
        auto nr = sscanf(start_coeff.c_str(), "%f,%f,%f,%f,%f,%f,%f,%f,%f",
            &coeff.coeff[0], &coeff.coeff[1], &coeff.coeff[2],
            &coeff.coeff[3], &coeff.coeff[4], &coeff.coeff[5],
            &coeff.coeff[6], &coeff.coeff[7], &coeff.coeff[8]);
        if (nr == 9) {
            calib.set_calibration(coeff);
        } else {
            printf("ERROR: wrong matrix; abort\n");
            exit(1);
        }
    }


    gui.set_add_click([&](int x, int y) -> bool{
        return calib.add_click(x, y);
    });
    gui.set_reset([&](){
        return calib.reset();
    });

    // wait for timer signal, processes events
    auto ret = gui.mainloop();

    if (!ret) {
        printf("No results.. exit\n");
        return 1;
    }

    if (verbose) {
        printf("Click points accepted:\n");
        for( auto [x, y] : gui.get_points())
            printf("\tx=%d, y=%d\n", x, y);
    }

    auto [w, h] = gui.get_display_size();

    calib.finish(w, h);

    if (show_matrix) {
        auto coeff = calib.get_coeff();
        printf("Calibration matrix:\n");
        mat9_print(coeff);
    }

    if (!not_save) {
        if (verbose)
            printf("Update the X11 calibration matrix\n");
        calib.save_calibration();
    }

    if (show_conf_x11 || output_file_x11.size())
        calib.output_xorgconfd(output_file_x11);
    if (show_conf_xinput || output_file_xinput.size())
        calib.output_xinput(output_file_xinput);

    return 0;
}
