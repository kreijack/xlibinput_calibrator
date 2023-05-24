/*
 * Copyright (c) 2020,2021,2022,2023 Goffredo Baroncelli
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

/*
 * Free inspired from https://github.com/freedesktop/xorg-xinput/blob/master/src/property.c
 */

#pragma once

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>


#include <map>
#include <string>
#include <vector>
#include <utility>

class XInputTouch {
public:
    XInputTouch(Display *display);

    ~XInputTouch();

    int find_touch(std::pair<XID,std::string> &ret);
    int list_props(int dev_id,
                        std::map<std::string, std::vector<std::string>> &ret);
    int set_prop(int devid, const char *name, Atom type, int format,
                        const std::vector<std::string> &values);
    int set_prop(int devid, const char *name,
            const std::vector<std::string> &values) {
                return set_prop(devid, name, 0, 0, values);
    }
    int get_prop(XDevice* dev, const char *name,
                        std::vector<std::string> &ret);
    int get_prop(int devid, const char *name,
                        std::vector<std::string> &ret);
    int has_prop(int devid, const std::string &prop_name);

    std::vector<std::pair<int, std::string>> list_devices();

private:

    Atom parse_atom(const char *name);
    Display *display;
    Atom xi_touchscreen;
    Atom xi_mouse;
    Atom xi_keyboard;
    Atom float_atom;
};
