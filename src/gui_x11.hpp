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

#include <vector>
#include <X11/Xlib.h>
#include <functional>
#include <utility>

enum { BLACK=0, WHITE=1, GRAY=2, DIMGRAY=3, RED=4 };
inline const int nr_colors = 5;
/*
 * Number of blocks. We partition the screen into 'num_blocks' x 'num_blocks'
 * rectangles of equal size. We then ask the user to press points that are
 * located at the corner closes to the center of the four blocks in the corners
 * of the screen. The following ascii art illustrates the situation. We partition
 * the screen into 8 blocks in each direction. We then let the user press the
 * points marked with 'O'.
 *
 *   +--+--+--+--+--+--+--+--+
 *   |  |  |  |  |  |  |  |  |
 *   +--O--+--+--+--+--+--O--+
 *   |  |  |  |  |  |  |  |  |
 *   +--+--+--+--+--+--+--+--+
 *   |  |  |  |  |  |  |  |  |
 *   +--+--+--+--+--+--+--+--+
 *   |  |  |  |  |  |  |  |  |
 *   +--+--+--+--+--+--+--+--+
 *   |  |  |  |  |  |  |  |  |
 *   +--+--+--+--+--+--+--+--+
 *   |  |  |  |  |  |  |  |  |
 *   +--+--+--+--+--+--+--+--+
 *   |  |  |  |  |  |  |  |  |
 *   +--O--+--+--+--+--+--O--+
 *   |  |  |  |  |  |  |  |  |
 *   +--+--+--+--+--+--+--+--+
 */

/*******************************************
 * X11 class for the the calibration GUI
 *******************************************/
class GuiCalibratorX11
{
public:
    ~GuiCalibratorX11();
    bool mainloop();
    GuiCalibratorX11(int monitor_nr = 1);

private:
    // Data
    double X[4], Y[4];
    int window_x, window_y, window_width, window_height;
    int time_elapsed;
    int points_count;
    bool return_value;
    bool do_loop;
    int monitor_nr = 0;
    const int num_blocks = 8;

    // X11 vars
    Display* display;
    int screen_num;
    Window win;
    GC gc;
    XFontStruct* font_info;
    // color mngmt
    unsigned long pixel[nr_colors];


    // event handlers
    void on_timer_signal();
    void on_xevent();
    void on_expose_event();
    void on_button_press_event(XEvent event);

    // Helper functions
    void set_window_size(int x, int y, int width, int height);
    void redraw();
    void draw_message(const char* msg);

    std::function<bool(int, int)> add_click_ext = [](int x, int y){ return true; };
    std::function<void(void)> reset_ext = [](){ };

    void get_monitor_size(int &x, int &y, int &w, int &h, int monitor_num = 0);

public:
    void set_add_click(std::function<bool(int, int)> f) {
        add_click_ext = f;
    }
    void set_reset(std::function<void(void)> f) {
        reset_ext = f;
    }

    std::pair<int, int> get_display_size() { return {window_width,
                                                        window_height}; }
};
