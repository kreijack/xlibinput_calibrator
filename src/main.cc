/*
 * Copyright (c) 2009 Tias Guns
 * Copyright (c) 2020,2021 Goffredo Baroncelli
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
#include <cstring>

#include "gui_x11.hpp"
#include "calibrator.hpp"
#include "xinput.hpp"

extern const char *gitversion;

void show_help() {
    printf("usage:\n"
        "xlibinput_calibrator [opts]\n"
        "    --output-file-x11-config=<filename>   save the output to filename\n"
        "    --output-file-xinput-cmd=<filename>   save the output to filename\n"
        "    --threshold-misclick=<nn>     set the threshold for misclick to <nn>\n"
        "    --threshold-doubleclick=<nn>  set the threshold for doubleckick to <nn>\n"
        "    --device-name=<devname>       set the touch screen device by name\n"
        "    --device-id=<devid>           set the touch screen device by id\n"
        "    --matrix-name=<matrix name>   set the calibration matrix name\n"
        "    --show-x11-config             show the config for X11\n"
        "    --show-xinput-cmd             show the config for libinput\n"
        "    --show-matrix                 show the final matrix\n"
        "    --verbose                     set verbose to on\n"
        "    --dont-save                   don't update X11 setting\n"
        "    --matrix=x1,x2..x9            start coefficent matrix\n"
        "    --monitor-number=<n>          show the output on the monitor '<n>'\n"
        "\n"
        "xlibinput_calibrator --list-devices       show the devices availables\n"
        "\n"
        "version: %s\n"
        "\n",
        gitversion
    );
}

#define CALMATR1 "libinput Calibration Matrix"
#define CALMATR2 "Coordinate Transformation Matrix"

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

static int list_devices(void) {
    XInputTouch xi;
    const auto devices = xi.list_devices();

    for (auto && it : devices) {
        std::map<std::string, std::vector<std::string>> props;
        printf("%3d: %s\n", it.first, it.second.c_str());

        xi.list_props(it.first, props);
        for (auto && it2 : props) {
            printf("\t%s: ", it2.first.c_str());
            for (auto i = 0u ; i < it2.second.size() ; i++) {
                if (i)
                    printf(", ");
                printf("%s", it2.second[i].c_str());
            }
            printf("\n");
        }

    }

    return 0;
}

int main(int argc, char** argv)
{

    std::string output_file_x11;
    std::string output_file_xinput;
    bool verbose = false;
    int thr_misclick = 0;
    int thr_doubleclick = 1;
    std::string device_name;
    XID device_id = (XID)-1;
    bool show_matrix = false;
    bool show_conf_x11 = false;
    bool show_conf_xinput = false;
    bool not_save = false;
    int monitor_nr = 0;
    std::string start_coeff;
    std::string matrix_name;

    if (argc == 2 && !strcmp(argv[1], "--list-devices"))
        return list_devices();

    for (int i = 1 ; i < argc ; i++) {
        const std::string arg{argv[i]};

        if (starts_with(arg, "--output-file-x11-config=")) {
            output_file_x11 = arg.substr(25);
        } else if (starts_with(arg, "--output-file-xinput-cmd=")) {
            output_file_xinput = arg.substr(25);
        } else if (starts_with(arg, "--monitor-number=")) {
            auto opt = arg.substr(17);
            if (opt == "all")
                monitor_nr = -1;
            else
                monitor_nr = stoi(opt);
        } else if (starts_with(arg, "--threshold-misclick=")) {
            thr_misclick = stoi(arg.substr(21));
        } else if (starts_with(arg, "--threshold-doubleclick=")) {
            thr_doubleclick = stoi(arg.substr(24));
        } else if (starts_with(arg, "--device-name=")) {
            device_name = std::string(arg.substr(14));
        } else if (starts_with(arg, "--matrix-name=")) {
            matrix_name = std::string(arg.substr(14));
        } else if (starts_with(arg, "--device-id=")) {
            device_id = stou(arg.substr(12));
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--dont-save") {
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

    XInputTouch xinputtouch;

    if (device_id == (XID)-1 && device_name == "") {
        std::pair<XID, std::string> dev;
        if (xinputtouch.find_touch(dev) < 0) {
            fprintf(stderr, "ERROR: Unable to find device\n");
            exit(100);
        }

        device_name = dev.second;
        device_id = dev.first;
    } else {
        // search a device
        const auto res = xinputtouch.list_devices();
        if (res.size() == 0) {
            fprintf(stderr, "ERROR: Unable to find device\n");
            exit(100);
        }

        for( auto i : res) {
            if (device_id != (XID)-1 && device_id == (XID)i.first) {
                device_name = i.second;
                break;
            } else if (device_name == i.second) {
                device_id = i.first;
                break;
            }
        }
    }

    if (device_id == (XID)-1 || device_name == "")  {
        fprintf(stderr, "ERROR: Unable to find device\n");
        exit(100);
    }

    if (verbose) {
        printf("device-id:                  %lu\n", device_id);
        printf("device-name:                '%s'\n", device_name.c_str());
    }

    // find a suitable calibration matrix
    std::map<std::string, std::vector<std::string>> lprops;
    if (xinputtouch.list_props(device_id, lprops) < 0) {
        fprintf(stderr, "ERROR: Unable to get the device properties\n");
        exit(100);
    }

    if (matrix_name == "") {
        for (auto i : lprops) {
            if (i.first == CALMATR1) {
                matrix_name = i.first;
                break;
            } else if (i.first == CALMATR2 && matrix_name == "") {
                matrix_name = i.first;
                // no break, continue to search CALMATR1 if available
            }
        }
    } else {
        bool found = false;
        for (auto i : lprops)
            if (i.first == matrix_name)
                found = true;
        if (!found) {
            fprintf(stderr, "ERROR: Unable to find a suitable calibration matrix\n");
            exit(100);
        }
    }

    if (matrix_name == "") {
        fprintf(stderr, "ERROR: Unable to find a suitable calibration matrix\n");
        exit(100);
    }

    if (verbose) {
        printf("show-matrix:                %s\n", show_matrix ? "yes" : "no");
        printf("show-x11-config:            %s\n", show_conf_x11 ? "yes" : "no");
        printf("show-libinput-config:       %s\n", show_conf_xinput ? "yes" : "no");
        printf("not-save:                   %s\n", show_conf_xinput ? "yes" : "no");
        printf("matrix-name:                '%s'\n", matrix_name.c_str());
        printf("output-file-x11-config:     '%s'\n", output_file_x11.c_str());
        printf("output-file-xinput-config:  '%s'\n", output_file_xinput.c_str());
        printf("threshold-misclick:         %d\n", thr_misclick);
        printf("threshold-doubleclick:      %d\n", thr_doubleclick);
        printf("monitor-number:             %d\n", monitor_nr);
    }


    GuiCalibratorX11 gui(monitor_nr);
    Calibrator  calib(device_name, device_id, thr_misclick, thr_doubleclick,
                        matrix_name, verbose);

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
            fprintf(stderr, "ERROR: wrong matrix; abort\n");
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
        for(int i = 0 ; i < calib.get_numclicks() ; i++) {
            auto [x, y] = calib.get_point(i);
            printf("\tx=%d, y=%d\n", x, y);
        }
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
