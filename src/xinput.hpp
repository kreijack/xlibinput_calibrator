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
    XInputTouch(const char *display) : XInputTouch(XOpenDisplay(display)) { }
    XInputTouch() : XInputTouch(getenv("DISPLAY")) { }

    XInputTouch(const XInputTouch &) = delete;
    XInputTouch(XInputTouch &&) = delete;
    XInputTouch & operator = (const XInputTouch &) = delete;
    XInputTouch & operator = (XInputTouch &&) = delete;
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

    std::vector<std::pair<int, std::string>> list_devices();

private:

    Atom parse_atom(const char *name);
    XInputTouch(Display *display);
    Display *display;
    Atom xi_touchscreen;
    Atom xi_mouse;
    Atom xi_keyboard;
    Atom float_atom;
};
