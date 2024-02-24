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
        "    --output-file-udev-libinput-cmd=<filename>     save the output to filename\n"
        "    --threshold-misclick=<nn>     set the threshold for misclick to <nn>\n"
        "    --threshold-doubleclick=<nn>  set the threshold for doubleckick to <nn>\n"
        "    --device-name=<devname>       set the touch screen device by name\n"
        "    --device-id=<devid>           set the touch screen device by id\n"
        "    --matrix-name=<matrix name>   set the calibration matrix name\n"
        "    --show-x11-config             show the config for X11\n"
        "    --show-xinput-cmd             show the config for xinput-libinput\n"
        "    --show-udev-libinput-cmd      show the config for udev-libinput\n"
        "    --show-matrix                 show the final matrix\n"
        "    --verbose                     set verbose to on\n"
        "    --dont-save                   don't update X11 setting\n"
        "    --start-matrix=x1,x2..x9      start coefficient matrix\n"
        "    --display=<display>           set the X11 display\n"
        "    --monitor-number=<n>          show the output on the monitor '<n>'\n"
        "\n"
        "xlibinput_calibrator --list-devices       show the devices availables\n"
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

static void print_device_not_found(const std::vector<XInputTouch::XDevInfo> &v) {
    printf("ERROR: Unable to find a default touchscreen to calibrate\n");
    if (v.size() > 1) {
        printf("ERROR: Possible alternatives:\n");
        for (auto &dev : v)
            printf("ERROR:     %llu - %s\n",
                   (unsigned long long)dev.id,
                   dev.name.c_str());
    }
}

static int list_devices(Display *display) {
    XInputTouch xi(display);

    for (auto &dev: xi.list_devices()) {
        std::map<std::string, std::vector<std::string>> props;
        printf("%3llu: %s\n", (unsigned long long)dev.id, dev.name.c_str());
        printf("\tType: %s\n", dev.type_str.c_str());
        xi.list_props(dev.id, props);
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

    std::vector<XInputTouch::XDevInfo> ret;
    if (xi.find_touch(ret) != 1) {
        printf("\n");
        print_device_not_found(ret);
    } else {
        printf("\nINFO: Default touchscreen: %llu - %s\n",
		(unsigned long long)ret[0].id, ret[0].name.c_str());
    }
    return 0;
}

int main(int argc, char** argv)
{

    std::string output_file_x11;
    std::string output_file_xinput;
    std::string output_file_udev_libinput;
    bool verbose = false;
    int thr_misclick = 0;
    int thr_doubleclick = 1;
    std::string device_name;
    XID device_id = (XID)-1;
    bool show_matrix = false;
    bool show_conf_x11 = false;
    bool show_conf_xinput = false;
    bool show_conf_udev_libinput = false;
    bool not_save = false;
    int monitor_nr = 0;
    std::string start_coeff;
    std::string matrix_name;
    std::string DisplayName = "";
    Display *display;
    bool start_list_devices = false;
    bool prescale_applied = false;

    if (getenv("DISPLAY"))
        DisplayName = getenv("DISPLAY");

    for (int i = 1 ; i < argc ; i++) {
        const std::string arg{argv[i]};

        if (starts_with(arg, "--output-file-x11-config=")) {
            output_file_x11 = arg.substr(25);
        } else if (starts_with(arg, "--output-file-xinput-cmd=")) {
            output_file_xinput = arg.substr(25);
        } else if (starts_with(arg, "--output-file-udev-libinput-cmd=")) {
            output_file_udev_libinput = arg.substr(32);
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
        } else if (starts_with(arg, "--display=")) {
            DisplayName = std::string(arg.substr(10));
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
        } else if (arg == "--show-udev-libinput-cmd") {
            show_conf_udev_libinput = true;
        } else if (arg == "--show-matrix") {
            show_matrix = true;
        } else if (starts_with(arg, "--start-matrix=")) {
            start_coeff = arg.substr(15);
        } else if (arg == "--list-devices") {
            start_list_devices = true;
        } else if (arg == "--help" || arg == "-h") {
            show_help();
            exit(0);
        } else {
            printf("ERROR: unknown parameter '%s'\n", argv[i]);
            show_help();
            exit(1);
        }
    }

    if (DisplayName == "") {
        fprintf(stderr, "ERROR: cannot find a valid DISPLAY to open\n");
        exit(1);
    }
    display = XOpenDisplay(DisplayName.c_str());
    if (!display) {
        fprintf(stderr, "ERROR: can't open display '%s'\n", DisplayName.c_str());
        exit(1);
    }

    if (start_list_devices)
        return list_devices(display);

    XInputTouch xinputtouch(display);

    if (device_id == (XID)-1 && device_name == "") {
        std::vector<XInputTouch::XDevInfo>  ret;
        if (xinputtouch.find_touch(ret) != 1) {
            print_device_not_found(ret);
            exit(100);
        }

        device_name = ret[0].name;
        device_id = ret[0].id;
    } else {
        // search a device
        const auto res = xinputtouch.list_devices();
        if (res.size() == 0) {
            fprintf(stderr, "ERROR: Unable to find device\n");
            exit(100);
        }

        for( auto &dev: res) {
            if (device_id != (XID)-1 && device_id == (XID)dev.id) {
                device_name = dev.name;
                break;
            } else if (device_name == dev.name) {
                device_id = dev.id;
                break;
            }
        }
    }

    if (device_id == (XID)-1 || device_name == "")  {
        fprintf(stderr, "ERROR: Unable to find device\n");
        exit(100);
    }

    if (verbose) {
        printf("device-id:                         %lu\n", device_id);
        printf("device-name:                       '%s'\n", device_name.c_str());
    }

    // find a suitable calibration matrix
    std::map<std::string, std::vector<std::string>> lprops;
    if (xinputtouch.list_props(device_id, lprops) < 0) {
        fprintf(stderr, "ERROR: Unable to get the device properties\n");
        exit(100);
    }

    if (matrix_name == "") {
        for (auto i : lprops) {
            if (i.first == LICALMATR) {
                matrix_name = i.first;
                break;
            } else if (i.first == XICALMATR && matrix_name == "") {
                matrix_name = i.first;
                // no break, continue to search LICALMATR if available
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
        printf("show-matrix:                       %s\n", show_matrix ? "yes" : "no");
        printf("show-x11-config:                   %s\n", show_conf_x11 ? "yes" : "no");
        printf("show-libinput-config:              %s\n", show_conf_xinput ? "yes" : "no");
        printf("show-udev-libinput-config:         %s\n", show_conf_udev_libinput ? "yes" : "no");
        printf("not-save:                          %s\n", not_save ? "yes" : "no");
        printf("matrix-name:                       '%s'\n", matrix_name.c_str());
        printf("output-file-x11-config:            '%s'\n", output_file_x11.c_str());
        printf("output-file-xinput-config:         '%s'\n", output_file_xinput.c_str());
        printf("output-file-udev-libinput-config:  '%s'\n", output_file_udev_libinput.c_str());
        printf("threshold-misclick:                %d\n", thr_misclick);
        printf("threshold-doubleclick:             %d\n", thr_doubleclick);
        printf("monitor-number:                    %d\n", monitor_nr);
    }


    GuiCalibratorX11 gui(display, monitor_nr);
    Calibrator  calib(display, device_name, device_id, thr_misclick, thr_doubleclick,
                        matrix_name, verbose);

    int monitor_x, monitor_y, monitor_width, monitor_height;
    int overall_width, overall_height;
    gui.get_overall_display_size(overall_width, overall_height);
    gui.get_monitor_size(monitor_x, monitor_y,
                         monitor_width, monitor_height, monitor_nr);

    if(verbose) {
	    printf("Calibrating for monitor size %d, %d at %d, %d on overall display of %d, %d\n",
                monitor_width, monitor_height, monitor_x, monitor_y, overall_width, overall_height);
    }

    if (start_coeff.size() == 0) {
        /* When multiple monitors are attached X translates the incoming clicks
         * to cover the whole display area across all monitors. This causes real
         * problems if the overall display isn't rectangular, such as when 2
         * monitors are different resolutions. In this case X11 won't generate a click
         * off the monitors, instead moving it to the nearest pixel (in the X direction?)
         * which is on a monitor. This means that if you have a 1024x768 monitor to the
         * left of a 1920x1080 monitor, all clicks in the bottom-left corner will
         * actually come in as clicks with an X co-ordinate of 1024 (the start of the
         * taller 1920x1080 monitor).
         *
         * To prevent this problem, we must first translate and scale the whole
         * co-ordinate space of the overall display width/height, into the co-ordinate
         * space of the monitor we're drawing our window on, that way all clicks will
         * be scaled to values X11 will actually return to our program.
         */
        Mat9 prescale;
        mat9_set_identity(prescale);

        prescale.set((float)monitor_width/overall_width, 0, (float)monitor_x/overall_width,
                     0, (float)monitor_height/overall_height, (float)monitor_y/overall_height,
                     0, 0, 1);

        if(verbose) {
            printf("Prescaled for multi-monitors: %f,%f,%f,%f\n",prescale[0],prescale[4],prescale[2],prescale[5]);
        }

        calib.set_calibration(prescale);
        prescale_applied = true;

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
        float x1, y1;
        if (prescale_applied) {
            x1 = (float)x/monitor_width * overall_width - monitor_x;
            y1 = (float)y/monitor_height * overall_height - monitor_y;
        } else {
            x1 = x;
            y1 = y;
        }
        return calib.add_click(x1, y1);
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

    calib.finish(monitor_width, monitor_height);

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
    if (show_conf_udev_libinput || output_file_udev_libinput.size())
        calib.output_udev_libinput(output_file_udev_libinput);

    return 0;
}
