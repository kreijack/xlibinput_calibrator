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

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h> // strncpy, strlen

#ifdef HAVE_X11_XRANDR
// support for multi-head setups
#include <X11/extensions/Xrandr.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#include <string>
#include <stdexcept>
#include <cassert>

#include "gui_x11.hpp"


/// Names of the points
enum {
    UL = 0, // Upper-left
    UR = 1, // Upper-right
    LL = 2, // Lower-left
    LR = 3,  // Lower-right
    NUM_POINTS
};

// Timeout parameters
static const int time_step = 100;  // in milliseconds
static const int max_time = 15000; // 5000 = 5 sec

// Clock appereance
static const int cross_lines = 25;
static const int cross_circle = 4;
static const int clock_radius = 50;
static const int clock_line_width = 10;

// Text printed on screen
static const int font_size = 16;
static const int help_lines = 4;
static const std::string help_text[help_lines] = {
    "Touchscreen Calibration",
    "Press the point, use a stylus to increase precision.",
    "",
    "(To abort, press any key or wait)"
};

// color management

static const char* colors[nr_colors] = {"BLACK", "WHITE", "GRAY", "DIMGRAY", "RED"};

GuiCalibratorX11::GuiCalibratorX11(int mnr)
  : time_elapsed(0), points_count(0), monitor_nr(mnr)
{
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        throw std::runtime_error("Unable to connect to X server");
    }
    screen_num = DefaultScreen(display);
    // Load font and get font information structure
    font_info = XLoadQueryFont(display, "9x15");
    if (font_info == NULL) {
        // fall back to native font
        font_info = XLoadQueryFont(display, "fixed");
        if (font_info == NULL) {
            XCloseDisplay(display);
            throw std::runtime_error("Unable to open neither '9x15' nor 'fixed' font");
        }
    }

    int x, y, w, h;
    get_monitor_size(x, y, w, h, monitor_nr);
    set_window_size(x, y, w, h);

    //printf("window: %d,%x x %d,%d\n", window_x, window_y, window_width, window_height);
    // Register events on the window
    XSetWindowAttributes attributes;
    attributes.override_redirect = True;
    attributes.event_mask = ExposureMask | KeyPressMask | ButtonPressMask;

    win = XCreateWindow(display, RootWindow(display, screen_num),
                window_x, window_y, window_width, window_height, 0,
                CopyFromParent, InputOutput, CopyFromParent,
                CWOverrideRedirect | CWEventMask,
                &attributes);
    XMapWindow(display, win);

    // Listen to events
    XGrabKeyboard(display, win, False, GrabModeAsync, GrabModeAsync,
                CurrentTime);
    XGrabPointer(display, win, False, ButtonPressMask, GrabModeAsync,
                GrabModeAsync, None, None, CurrentTime);

    Colormap colormap = DefaultColormap(display, screen_num);
    XColor color;
    for (int i = 0; i != nr_colors; i++) {
        XParseColor(display, colormap, colors[i], &color);
        XAllocColor(display, colormap, &color);
        pixel[i] = color.pixel;
    }
    XSetWindowBackground(display, win, pixel[GRAY]);
    XClearWindow(display, win);

    gc = XCreateGC(display, win, 0, NULL);
    XSetFont(display, gc, font_info->fid);

}

void GuiCalibratorX11::get_monitor_size(int &x, int &y, int &w, int &h,
                                        int monitor_num) {

#ifndef HAVE_X11_XRANDR

    x = y = 0;
    w = DisplayWidth(display, screen_num);
    h = DisplayHeight(display, screen_num);
    return;

#else

    int n;
    auto root = RootWindow (display, screen_num);
    auto monitors = XRRGetMonitors(display, root, false, &n);

    if (n == -1) {
        fprintf(stderr, "WARNING: cannot execute XRRGetMonitors\n");
        x = y = 0;
        w = DisplayWidth(display, screen_num);
        h = DisplayHeight(display, screen_num);
        return;
    }

    if (monitor_num >= n || monitor_num < 0 || monitor_num == -1) {
        x = y = w = h = 0;

        for (int i = 0 ; i < n ; i++) {
            int width = monitors[i].x + monitors[i].width;
            int height = monitors[i].y + monitors[i].height;
            if (width > w)
                w = width;
            if (height > h)
                h = height;
        }

    } else {
        x = monitors[monitor_num].x;
        y = monitors[monitor_num].y;
        w = monitors[monitor_num].width;
        h = monitors[monitor_num].height;
    }

    XRRFreeMonitors(monitors);
#endif
}

GuiCalibratorX11::~GuiCalibratorX11()
{
    XUngrabPointer(display, CurrentTime);
    XUngrabKeyboard(display, CurrentTime);
    XFreeGC(display, gc);
    XCloseDisplay(display);
}

void GuiCalibratorX11::set_window_size(int x, int y, int width, int height) {
    window_width = width;
    window_height = height;
    window_x = x;
    window_y = y;

    // Compute absolute circle centers
    const int delta_x = window_width / num_blocks;
    const int delta_y = window_height / num_blocks;
    X[UL] = delta_x;                    Y[UL] = delta_y;
    X[UR] = window_width - delta_x - 1; Y[UR] = delta_y;
    X[LL] = delta_x;                    Y[LL] = window_height - delta_y - 1;
    X[LR] = window_width - delta_x - 1; Y[LR] = window_height - delta_y - 1;

    // reset calibration if already started
    points_count = 0;
}

void GuiCalibratorX11::redraw()
{


#if 0
    {
    int x, y, w, h;
    get_monitor_size(x, y, w, h, monitor_nr);
    if (x != window_x || y != window_y || w != window_width || h != window_height) {

          XMoveWindow(display, win, x, y);
          XResizeWindow(display, win, w, h);
          set_window_size(x, y, w, h);
    }
    }
#endif

    // Print the text
    int text_height = font_info->ascent + font_info->descent;
    int text_width = -1;
    for (int i = 0; i != help_lines; i++) {
        text_width = std::max(text_width, XTextWidth(font_info,
            help_text[i].c_str(), help_text[i].length()));
    }

    int x = (window_width - text_width) / 2;
    int y = (window_height - text_height) / 2 - 60;
    XSetForeground(display, gc, pixel[BLACK]);
    XSetLineAttributes(display, gc, 2, LineSolid, CapRound, JoinRound);
    XDrawRectangle(display, win, gc, x - 10, y - (help_lines*text_height) - 10,
                text_width + 20, (help_lines*text_height) + 20);

    // Print help lines
    y -= 3;
    for (int i = help_lines-1; i != -1; i--) {
        int w = XTextWidth(font_info, help_text[i].c_str(), help_text[i].length());
        XDrawString(display, win, gc, x + (text_width-w)/2, y,
                help_text[i].c_str(), help_text[i].length());
        y -= text_height;
    }

    // Draw the points
    for (int i = 0; i <= points_count; i++) {
        // set color: already clicked or not
        if (i < points_count)
            XSetForeground(display, gc, pixel[WHITE]);
        else
            XSetForeground(display, gc, pixel[RED]);
        XSetLineAttributes(display, gc, 1, LineSolid, CapRound, JoinRound);

        XDrawLine(display, win, gc, X[i] - cross_lines, Y[i],
                X[i] + cross_lines, Y[i]);
        XDrawLine(display, win, gc, X[i], Y[i] - cross_lines,
                X[i], Y[i] + cross_lines);
        XDrawArc(display, win, gc, X[i] - cross_circle, Y[i] - cross_circle,
                (2 * cross_circle), (2 * cross_circle), 0, 360 * 64);
    }

    // Draw the clock background
    XSetForeground(display, gc, pixel[DIMGRAY]);
    XSetLineAttributes(display, gc, 0, LineSolid, CapRound, JoinRound);
    XFillArc(display, win, gc, (window_width-clock_radius)/2, (window_height - clock_radius)/2,
                clock_radius, clock_radius, 0, 360 * 64);
}

void GuiCalibratorX11::on_expose_event()
{
    redraw();
}

void GuiCalibratorX11::on_timer_signal()
{
    time_elapsed += time_step;
    if (time_elapsed > max_time) {
        do_loop = false;
        return_value = false;
        return;
    }

    // Update clock
    XSetForeground(display, gc, pixel[BLACK]);
    XSetLineAttributes(display, gc, clock_line_width,
                LineSolid, CapButt, JoinMiter);
    XDrawArc(display, win, gc, (window_width-clock_radius+clock_line_width)/2,
                (window_height-clock_radius+clock_line_width)/2,
                clock_radius-clock_line_width, clock_radius-clock_line_width,
                90*64, ((double)time_elapsed/(double)max_time) * -360 * 64);
}

void GuiCalibratorX11::on_button_press_event(XEvent event)
{
    // Clear window, maybe a bit overdone, but easiest for me atm.
    // (goal is to clear possible message and other clicks)
    XClearWindow(display, win);

    // Handle click
    time_elapsed = 0;
    bool success = add_click_ext(event.xbutton.x, event.xbutton.y);

    if (!success) {
        draw_message("Mis-click detected, restarting...");
        points_count = 0;
        reset_ext();
    } else {
        points_count ++;
    }

    // Are we done yet?
    if (points_count >= 4) {
        return_value = true;
        do_loop = false;
        return;
    }

    // Force a redraw
    redraw();
}

void GuiCalibratorX11::draw_message(const char* msg)
{
    int text_height = font_info->ascent + font_info->descent;
    int text_width = XTextWidth(font_info, msg, strlen(msg));

    int x = (window_width - text_width) / 2;
    int y = (window_height - text_height) / 2 + clock_radius + 60;
    XSetForeground(display, gc, pixel[BLACK]);
    XSetLineAttributes(display, gc, 2, LineSolid, CapRound, JoinRound);
    XDrawRectangle(display, win, gc, x - 10, y - text_height - 10,
                text_width + 20, text_height + 25);

    XDrawString(display, win, gc, x, y, msg, strlen(msg));
}

void GuiCalibratorX11::on_xevent()
{
    // process events
    XEvent event;
    while (XCheckWindowEvent(display, win, -1, &event) == True) {
        switch (event.type) {
            case Expose:
                // only draw the last contiguous expose
                if (event.xexpose.count != 0)
                    break;
                on_expose_event();
                break;

            case ButtonPress:
                on_button_press_event(event);
                break;

            case KeyPress:
                /* FIXME */
                return_value = false;
                do_loop = false;
                return;
                break;
        }
    }
}

bool GuiCalibratorX11::mainloop() {

    // This returns the FD of the X11 display (or something like that)
    auto x11_fd = ConnectionNumber(display);

    do_loop = true;

    // Main loop
    while(do_loop) {
        fd_set in_fds;
        struct timeval tv;

        tv.tv_sec = time_step / 1000;
        tv.tv_usec = (time_step % 1000) * 1000;

        // Create a File Description Set containing x11_fd
        FD_ZERO(&in_fds);
        FD_SET(x11_fd, &in_fds);

        // Wait for X Event or a Timer
        if (!select(x11_fd+1, &in_fds, 0, 0, &tv))
            on_timer_signal();
        on_xevent();
    }

    return return_value;
}
