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

XInputTouch::XInputTouch(Display *display_) {
    display = display_;
    xi_touchscreen = XInternAtom(display, XI_TOUCHSCREEN, false);
    xi_mouse = XInternAtom(display, XI_MOUSE, false);
    xi_keyboard = XInternAtom(display, XI_KEYBOARD, false);
    float_atom = XInternAtom(display, "FLOAT", false);
}

XInputTouch::~XInputTouch() {
}

int XInputTouch::find_touch(std::pair<XID,std::string> &ret)

{
    XDeviceInfo	*devices;
    std::pair<XID,std::string> ret_ts, ret_cal;
    int         found_touchscreen = 0;
    int         found_calib_matrix = 0;
    int		loop;
    int		num_devices;

    devices = XListInputDevices(display, &num_devices);

    for (loop=0; loop<num_devices; loop++) {

        if (!devices[loop].type)
            continue;

        if (devices[loop].type == xi_touchscreen) {
            found_touchscreen++;
            ret_ts = {devices[loop].id, devices[loop].name};
        }
        //Fall back on checking if device has a calibration matrix.
        //Some devices don't report as touchscreens, but have a calibration matrix.
        else if(has_prop(devices[loop].id, "libinput Calibration Matrix") == 0) {
            found_calib_matrix++;
            ret_cal = {devices[loop].id, devices[loop].name};
        }

    }

    XFreeDeviceList(devices);

    //Return -1 unless we find exactly one device, in which case we return 0
    if(found_touchscreen > 0)
    {
        ret = ret_ts;
        if(found_touchscreen > 1)
            return -1;
        return 0;
    }
    else if(found_calib_matrix > 0)
    {
        ret = ret_cal;
        if(found_calib_matrix > 1)
            return -1;
        return 0;
    }
    return -1;

}

std::vector<std::pair<int, std::string>> XInputTouch::list_devices()

{
    XDeviceInfo	*devices;
    int		loop;
    int		num_devices;

    std::vector<std::pair<int, std::string>> ret;

    devices = XListInputDevices(display, &num_devices);

    for (loop=0; loop<num_devices; loop++) {
        if (!devices[loop].type)
            continue;
        ret.push_back({devices[loop].id, devices[loop].name});
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
    XDevice     *dev;
    int         nprops;
    Atom        *props;
    int         ret = -1;

    dev = XOpenDevice(display, dev_id);
    if (!dev)
    {
        fprintf(stderr, "unable to open device '%d'\n", dev_id);
        return -2;
    }

    props = XListDeviceProperties(display, dev, &nprops);

    while(nprops--) {
        auto name = XGetAtomName(display, props[nprops]);
        if (name == prop_name)
            ret = 0;
        XFree(name);
        if (ret == 0)
            break;
    }

    XFree(props);
    XCloseDevice(display, dev);

    return ret;
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

    xi.set_prop(devid, "libinput Calibration Matrix", {
        "1.0", "1.0", "1.0", "1.0", "0.5", "1.0", "0", "0", "1"
    });

    xi.get_prop(devid, "libinput Calibration Matrix", r1);
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
