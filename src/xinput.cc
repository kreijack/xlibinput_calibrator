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

#include <cstdio>
#include <cstring>
#include <cctype>
#include <cstdlib>

#include "xinput.hpp"

std::string XInputTouch::type_to_string(Atom type) {

        if (type == xi_mouse)
            return "MOUSE";
        if (type == xi_tablet)
            return "TABLET";
        if (type == xi_keyboard)
            return "KEYBOARD";
        if (type == xi_touchscreen)
            return "TOUCHSCREEN";
        if (type == xi_touchpad)
            return "TOUCHPAD";
        if (type == xi_buttonbox)
            return "BUTTONBOX";
        if (type == xi_barcode)
            return "BARCODE";
        if (type == xi_trackball)
            return "TRACKBALL";
        if (type == xi_quadrature)
            return "QUADRATURE";
        if (type == xi_id_module)
            return "ID_MODULE";
        if (type == xi_one_knob)
            return "ONE_KNOB";
        if (type == xi_nine_knob)
            return "NINE_KNOB";
        if (type == xi_knob_box)
            return "KNOB_BOX";
        if (type == xi_spaceball)
            return "SPACEBALL";
        if (type == xi_dataglove)
            return "DATAGLOVE";
        if (type == xi_eyetracker)
            return "EYETRACKER";
        if (type == xi_cursorkeys)
            return "CURSORKEYS";
        if (type == xi_footmouse)
            return "FOOTMOUSE";
        if (type == xi_joystick)
            return "JOYSTICK";

	static char buf[100];
	sprintf(buf, "<UNKNOWN:%llu>", (unsigned long long)type);
	return buf;
}

XInputTouch::XInputTouch(Display *display_) {
    display = display_;
    xi_touchscreen = XInternAtom(display, XI_TOUCHSCREEN, false);
    xi_mouse = XInternAtom(display, XI_MOUSE, false);
    xi_keyboard = XInternAtom(display, XI_KEYBOARD, false);
    float_atom = XInternAtom(display, "FLOAT", false);
    xi_tablet = XInternAtom(display, XI_TABLET, false);
    xi_touchpad = XInternAtom(display, XI_TOUCHPAD, false);
    xi_buttonbox = XInternAtom(display, XI_BUTTONBOX, false);
    xi_barcode = XInternAtom(display, XI_BARCODE, false);
    xi_trackball = XInternAtom(display, XI_TRACKBALL, false);
    xi_quadrature = XInternAtom(display, XI_QUADRATURE, false);
    xi_id_module = XInternAtom(display, XI_ID_MODULE, false);
    xi_one_knob = XInternAtom(display, XI_ONE_KNOB, false);
    xi_nine_knob = XInternAtom(display, XI_NINE_KNOB, false);
    xi_knob_box = XInternAtom(display, XI_KNOB_BOX, false);
    xi_spaceball = XInternAtom(display, XI_SPACEBALL, false);
    xi_dataglove = XInternAtom(display, XI_DATAGLOVE, false);
    xi_eyetracker = XInternAtom(display, XI_EYETRACKER, false);
    xi_cursorkeys = XInternAtom(display, XI_CURSORKEYS, false);
    xi_footmouse = XInternAtom(display, XI_FOOTMOUSE, false);
    xi_joystick = XInternAtom(display, XI_JOYSTICK, false);
}

XInputTouch::~XInputTouch() {
}

int XInputTouch::find_touch(std::vector<XInputTouch::XDevInfo> &ret) {
    int count = 0;

    for (auto  &dev: list_devices()) {

        /* Can't query properties of devices that have no type at all,
         * so check that first. */
        if (!dev.type)
            continue;
        /* Some touchscreens identify as xi_mouse, but still have a
         * calibration matrix, so check for that as well as just
         * xi_touchscreen
         */
        if (dev.type != xi_touchscreen &&
            has_prop(dev.id, LICALMATR) != 0)
                continue;

        ret.push_back(dev);
        ++count;
    }

    return count;
}

std::vector<XInputTouch::XDevInfo> XInputTouch::list_devices()

{
    XDeviceInfo	*devices;
    int		loop;
    int		num_devices;

    std::vector<XInputTouch::XDevInfo> ret;

    devices = XListInputDevices(display, &num_devices);

    for (loop=0; loop<num_devices; loop++) {
        if (!devices[loop].type)
            continue;
        ret.push_back({devices[loop].name,
		devices[loop].id,
		devices[loop].type,
		type_to_string(devices[loop].type)});
    }

    XFreeDeviceList(devices);

    return ret;
}

Atom XInputTouch::parse_atom(const char *name) {
    Bool is_atom = True;
    int i;

    for (i = 0; name[i] != '\0'; i++) {
        if (!isdigit(name[i])) {
            is_atom = False;
            break;
        }
    }

    if (is_atom)
        return atoi(name);
    else
        return XInternAtom(display, name, False);
}

int XInputTouch::get_prop(int devid, const char *pname,
                    std::vector<std::string> &ret)
{
    auto dev = XOpenDevice(display, devid);
    if (!dev)
    {
        fprintf(stderr, "unable to open device '%d'\n", devid);
        return -2;
    }
    auto r = get_prop(dev, pname, ret);
    XCloseDevice(display, dev);

    return r;
}
int XInputTouch::get_prop(XDevice* dev, const char *pname,
                    std::vector<std::string> &ret)
{
    Atom                act_type, property;
    char                *name;
    int                 act_format;
    unsigned long       nitems, bytes_after;
    unsigned char       *data, *ptr;
    int                 j, done = False, size = 0;


    property = parse_atom(pname);

    if (property == None) {
        fprintf(stderr, "invalid property '%s'\n", pname);
        return -1;
    }

    if (XGetDeviceProperty(display, dev, property, 0, 1000, False,
                           AnyPropertyType, &act_type, &act_format,
                           &nitems, &bytes_after, &data) != Success)
        return -2;

    if (nitems==0)
        return -4;

    ptr = data;

    switch(act_format)
    {
        case 8: size = sizeof(char); break;
        case 16: size = sizeof(short); break;
        case 32: size = sizeof(long); break;
    }

    for (j = 0; j < (int)nitems; j++)
    {
        std::string next_value;

        switch(act_type)
        {
            case XA_INTEGER:
                switch(act_format)
                {
                    case 8:
                        next_value = std::to_string(*((char*)ptr));
                        break;
                    case 16:
                        next_value = std::to_string(*((short*)ptr));
                        break;
                    case 32:
                        next_value = std::to_string(*((long*)ptr));
                        break;
                }
                break;
            case XA_CARDINAL:
                switch(act_format)
                {
                    case 8:
                        next_value = std::to_string(*((unsigned char*)ptr));
                        break;
                    case 16:
                        next_value = std::to_string(*((unsigned short*)ptr));
                        break;
                    case 32:
                        next_value = std::to_string(*((unsigned long*)ptr));
                        break;
                }
                break;
            case XA_STRING:
                if (act_format != 8)
                {
                    next_value = "<Unknown string format>";
                    done = True;
                    break;
                }
                next_value = std::string((char*)ptr);
                j += strlen((char*)ptr); /* The loop's j++ jumps over the
                                            terminating 0 */
                ptr += strlen((char*)ptr); /* ptr += size below jumps over
                                              the terminating 0 */
                break;
            case XA_ATOM:
                {
                    Atom a = *(Atom*)ptr;
                    name = (a) ? XGetAtomName(display, a) : NULL;
                    if (name)
                        next_value = name;
                    else
                        next_value = std::to_string(a);
                    XFree(name);
                    break;
                }
            default:
                if (float_atom != None && act_type == float_atom)
                {
                    next_value = std::to_string(*((float*)ptr));
                    break;
                }

                name = XGetAtomName(display, act_type);
                next_value = "<unknown type: '";
                next_value += name;
                next_value += "'>";
                XFree(name);
                done = True;
                break;
        }

        ptr += size;

        ret.push_back(next_value);
        if (done == True)
            break;
    }

    XFree(data);
    return 0;
}

int
XInputTouch::list_props(int dev_id,
        std::map<std::string, std::vector<std::string>> &ret)
{
    XDevice     *dev;
    int         nprops;
    Atom        *props;

    dev = XOpenDevice(display, dev_id);
    if (!dev)
    {
        fprintf(stderr, "unable to open device '%d'\n", dev_id);
        return -2;
    }

    props = XListDeviceProperties(display, dev, &nprops);
    if (!nprops)
    {
        //printf("Device '%d' does not report any properties.\n", dev_id);
        return 0;
    }

    ret.clear();
    while(nprops--) {
        auto name = XGetAtomName(display, props[nprops]);
        ret[name] = {};
        get_prop(dev, name, ret[name]);
        XFree(name);
    }

    XFree(props);
    XCloseDevice(display, dev);

    return 0;
}

int
XInputTouch::has_prop(int dev_id, const std::string &prop_name)
{
    std::map<std::string, std::vector<std::string>> ret;

    auto r = list_props(dev_id, ret);

    if (r < 0)
        return r;

    return ret.count(prop_name) ? 0 : -1;
}

int XInputTouch::set_prop(int devid, const char *name, Atom type, int format,
                        const std::vector<std::string> &values)
{
    XDevice      *dev;
    Atom          prop;
    Atom          old_type;
    int           i;
    int           old_format, nelements = 0;
    unsigned long act_nitems, bytes_after;
    union {
        unsigned char *c;
        short *s;
        long *l;
        Atom *a;
    } data;


    prop = parse_atom(name);

    if (prop == None) {
        fprintf(stderr, "invalid property '%s'\n", name);
        return -1;
    }

    dev = XOpenDevice(display, devid);
    if (!dev)
    {
        fprintf(stderr, "unable to open device '%d'\n", devid);
        return -2;
    }
    nelements = values.size();
    if (type == None || format == 0) {
        if (XGetDeviceProperty(display, dev, prop, 0, 0, False,
                               AnyPropertyType,
                               &old_type, &old_format, &act_nitems,
                               &bytes_after, &data.c) != Success) {
            fprintf(stderr, "failed to get property type and format for '%s'\n",
                    name);
            return -2;
        } else {
            if (type == None)
                type = old_type;
            if (format == 0)
                format = old_format;
        }

        XFree(data.c);
    }

    if (type == None) {
        fprintf(stderr, "property '%s' doesn't exist, you need to specify "
                "its type and format\n", name);
        return -3;
    }

    data.c = (unsigned char *)calloc(nelements, sizeof(long));

    for (i = 0; i < nelements; i++)
    {
        if (type == XA_INTEGER || type == XA_CARDINAL) {
            switch (format)
            {
                case 8:
                    data.c[i] = stoi(values[i]);
                    break;
                case 16:
                    data.s[i] = stoi(values[i]);
                    break;
                case 32:
                    data.l[i] = stoi(values[i]);
                    break;
                default:
                    fprintf(stderr, "unexpected size for property '%s'", name);
                    return EXIT_FAILURE;
            }
        } else if (type == float_atom) {
            if (format != 32) {
                fprintf(stderr, "unexpected format %d for property '%s'\n",
                        format, name);
                return -3;
            }
            *(float *)(data.l + i) = stod(values[i]);
            /*FIXME: add a format check
             * if (endptr == argv[2 + i]) {
                fprintf(stderr, "argument '%s' could not be parsed\n", argv[2 + i]);
                return -2;
            }*/
        } else if (type == XA_ATOM) {
            if (format != 32) {
                fprintf(stderr, "unexpected format %d for property '%s'\n",
                        format, name);
                return -4;
            }
            data.a[i] = parse_atom(values[i].c_str());
        } else {
            fprintf(stderr, "unexpected type for property '%s'\n", name);
            return -5;
        }
    }

    XChangeDeviceProperty(display, dev, prop, type, format, PropModeReplace,
                          data.c, nelements);
    free(data.c);
    XSync(display, False);
    XCloseDevice(display, dev);
    return 0;
}

#ifdef DEBUG

#include <cassert>
#include <cstdlib>

int main() {
    XInputTouch xi;
    auto devid = xi.find_touch();
    fprintf(stderr, "touchid = %d\n",devid);

    std::map<std::string, std::vector<std::string>> ret;
    if (devid >= 0) {
        xi.list_props(devid, ret);

        for (const auto & [k, vs] : ret) {
            fprintf(stderr, "%s: ", k.c_str());
            bool first = true;
            for(const auto &v : vs) {
                if (!first)
                    fprintf(stderr, ", ");
                first = false;
                fprintf(stderr, "%s", v.c_str());
            }
            fprintf(stderr, "\n");
        }
    }
    std::vector<std::string> r1;
    auto r4 = xi.get_prop(devid, "lixxbinput Calibration Matrix__error", r1);
    fprintf(stderr, "r4=%d\n",r4);
    assert(r4 < 0);

    xi.set_prop(devid, LICALMATR, {
        "1.0", "1.0", "1.0", "1.0", "0.5", "1.0", "0", "0", "1"
    });

    xi.get_prop(devid, LICALMATR, r1);
    bool first;
    for(const auto &v : r1) {
        if (!first)
            fprintf(stderr, ", ");
        first = false;
        fprintf(stderr, "%s", v.c_str());
    }
    fprintf(stderr, "\n");
}

#endif
