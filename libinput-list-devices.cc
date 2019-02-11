/*
 * This code is derived from libinput sources:
 * 			tools/shared.c
 * 			tools/libinput-list-devices.c
 */

/*
 * Copyright © 2015 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*#include "config.h"*/

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libudev.h>
#include <linux/input.h>
#include <libinput.h>
/*#include <libinput-util.h>
#include <libinput-version.h>*/

#include "libinput-list-devices.hpp"

/*#include "shared.h"*/

using LMSS = std::list<std::map<std::string, std::string>>;
using MSS = std::map<std::string, std::string>;

static const char *
tap_default(struct libinput_device *device)
{
	if (!libinput_device_config_tap_get_finger_count(device))
		return "n/a";

	if (libinput_device_config_tap_get_default_enabled(device))
		return "enabled";
	else
		return "disabled";
}

static const char *
drag_default(struct libinput_device *device)
{
	if (!libinput_device_config_tap_get_finger_count(device))
		return "n/a";

	if (libinput_device_config_tap_get_default_drag_enabled(device))
		return "enabled";
	else
		return "disabled";
}

static const char *
draglock_default(struct libinput_device *device)
{
	if (!libinput_device_config_tap_get_finger_count(device))
		return "n/a";

	if (libinput_device_config_tap_get_default_drag_lock_enabled(device))
		return "enabled";
	else
		return "disabled";
}

static const char*
left_handed_default(struct libinput_device *device)
{
	if (!libinput_device_config_left_handed_is_available(device))
		return "n/a";

	if (libinput_device_config_left_handed_get_default(device))
		return "enabled";
	else
		return "disabled";
}

static const char *
nat_scroll_default(struct libinput_device *device)
{
	if (!libinput_device_config_scroll_has_natural_scroll(device))
		return "n/a";

	if (libinput_device_config_scroll_get_default_natural_scroll_enabled(device))
		return "enabled";
	else
		return "disabled";
}

static const char *
middle_emulation_default(struct libinput_device *device)
{
	if (!libinput_device_config_middle_emulation_is_available(device))
		return "n/a";

	if (libinput_device_config_middle_emulation_get_default_enabled(device))
		return "enabled";
	else
		return "disabled";
}

static char *
calibration_default(struct libinput_device *device)
{
	char *str;
	float calibration[6];

	if (!libinput_device_config_calibration_has_matrix(device)) {
		asprintf(&str, "n/a");
		return str;
	}

	libinput_device_config_calibration_get_default_matrix(device,
						  calibration);
	asprintf(&str,
		 "%.2f %.2f %.2f %.2f %.2f %.2f",
		 calibration[0],
		 calibration[1],
		 calibration[2],
		 calibration[3],
		 calibration[4],
		 calibration[5]);
	return str;
}

static char *
scroll_defaults(struct libinput_device *device)
{
	uint32_t scroll_methods;
	char *str;
	enum libinput_config_scroll_method method;

	scroll_methods = libinput_device_config_scroll_get_methods(device);
	if (scroll_methods == LIBINPUT_CONFIG_SCROLL_NO_SCROLL) {
		asprintf(&str, "none");
		return str;
	}

	method = libinput_device_config_scroll_get_default_method(device);

	asprintf(&str,
		 "%s%s%s%s%s%s",
		 (method == LIBINPUT_CONFIG_SCROLL_2FG) ? "*" : "",
		 (scroll_methods & LIBINPUT_CONFIG_SCROLL_2FG) ? "two-finger " : "",
		 (method == LIBINPUT_CONFIG_SCROLL_EDGE) ? "*" : "",
		 (scroll_methods & LIBINPUT_CONFIG_SCROLL_EDGE) ? "edge " : "",
		 (method == LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN) ? "*" : "",
		 (scroll_methods & LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN) ? "button" : "");
	return str;
}

static char*
click_defaults(struct libinput_device *device)
{
	uint32_t click_methods;
	char *str;
	enum libinput_config_click_method method;

	click_methods = libinput_device_config_click_get_methods(device);
	if (click_methods == LIBINPUT_CONFIG_CLICK_METHOD_NONE) {
		asprintf(&str, "none");
		return str;
	}

	method = libinput_device_config_click_get_default_method(device);
	asprintf(&str,
		 "%s%s%s%s",
		 (method == LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS) ? "*" : "",
		 (click_methods & LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS) ? "button-areas " : "",
		 (method == LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER) ? "*" : "",
		 (click_methods & LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER) ? "clickfinger " : "");
	return str;
}

static char*
accel_profiles(struct libinput_device *device)
{
	uint32_t profiles;
	char *str;
	enum libinput_config_accel_profile profile;

	if (!libinput_device_config_accel_is_available(device)) {
		asprintf(&str, "n/a");
		return str;
	}

	profiles = libinput_device_config_accel_get_profiles(device);
	if (profiles == LIBINPUT_CONFIG_ACCEL_PROFILE_NONE) {
		asprintf(&str, "none");
		return str;
	}

	profile = libinput_device_config_accel_get_default_profile(device);
	asprintf(&str,
		  "%s%s %s%s",
		  (profile == LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT) ? "*" : "",
		  (profiles & LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT) ? "flat" : "",
		  (profile == LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE) ? "*" : "",
		  (profiles & LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE) ? "adaptive" : "");

	return str;
}

static const char *
dwt_default(struct libinput_device *device)
{
	if (!libinput_device_config_dwt_is_available(device))
		return "n/a";

	if (libinput_device_config_dwt_get_default_enabled(device))
		return "enabled";
	else
		return "disabled";
}

static char *
rotation_default(struct libinput_device *device)
{
	char *str;
	double angle;

	if (!libinput_device_config_rotation_is_available(device)) {
		asprintf(&str, "n/a");
		return str;
	}

	angle = libinput_device_config_rotation_get_angle(device);
	asprintf(&str, "%.1f", angle);
	return str;
}

static void
print_pad_info(struct libinput_device *device,MSS &ret)
{
	int nbuttons, nrings, nstrips, ngroups, nmodes;
	struct libinput_tablet_pad_mode_group *group;

	nbuttons = libinput_device_tablet_pad_get_num_buttons(device);
	nrings = libinput_device_tablet_pad_get_num_rings(device);
	nstrips = libinput_device_tablet_pad_get_num_strips(device);
	ngroups = libinput_device_tablet_pad_get_num_mode_groups(device);

	group = libinput_device_tablet_pad_get_mode_group(device, 0);
	nmodes = libinput_tablet_pad_mode_group_get_num_modes(group);

	ret.insert({"Pad-Rings", std::to_string(nrings)});
	ret.insert({"Pad-Strips", std::to_string(nstrips)});
	ret.insert({"Pad-Buttons", std::to_string(nbuttons)});
	ret.insert({"Pad-Mode-groups", std::to_string(ngroups) + " ("+
			std::to_string(nmodes)+")"});
}

static MSS
fill_device_notify(struct libinput_device *dev)
{

	struct libinput_seat *seat = libinput_device_get_seat(dev);
	struct libinput_device_group *group;
	struct udev_device *udev_device;
	double w, h;
	static int next_group_id = 0;
	intptr_t group_id;
	const char *devnode;
	char *str;

	MSS ret;

	group = libinput_device_get_device_group(dev);
	group_id = (intptr_t)libinput_device_group_get_user_data(group);
	if (!group_id) {
		group_id = ++next_group_id;
		libinput_device_group_set_user_data(group, (void*)group_id);
	}

	udev_device = libinput_device_get_udev_device(dev);
	devnode = udev_device_get_devnode(udev_device);

	ret.insert({"Device", libinput_device_get_name(dev)});
	ret.insert({"Kernel", devnode});
	ret.insert({"Group", std::to_string((int)group_id)});
	std::string s;
	char buf[100];
	s = libinput_seat_get_physical_name(seat);
	s += ", ";
	s += libinput_seat_get_logical_name(seat);
	ret.insert({"Seat", s});
	udev_device_unref(udev_device);

	if (libinput_device_get_size(dev, &w, &h) == 0) {
		sprintf(buf, "%.fx%.fmm\n", w, h);
		ret.insert({"Size", buf});
	}

	s = "";
	if (libinput_device_has_capability(dev,
					   LIBINPUT_DEVICE_CAP_KEYBOARD))
		s += "keyboard ";
	if (libinput_device_has_capability(dev,
					   LIBINPUT_DEVICE_CAP_POINTER))
		s += "pointer ";
	if (libinput_device_has_capability(dev,
					   LIBINPUT_DEVICE_CAP_TOUCH))
		s += "touch ";
	if (libinput_device_has_capability(dev,
					   LIBINPUT_DEVICE_CAP_TABLET_TOOL))
		s += "tablet ";
	if (libinput_device_has_capability(dev,
					   LIBINPUT_DEVICE_CAP_TABLET_PAD))
		s += "tablet-pad ";
	if (libinput_device_has_capability(dev,
					   LIBINPUT_DEVICE_CAP_GESTURE))
		s += "gesture ";
	if (libinput_device_has_capability(dev,
					   LIBINPUT_DEVICE_CAP_SWITCH))
		s += "switch ";
	if (s.size())
		s = s.substr(0, s.size()-1);

	ret.insert({"Capabilities", s});


	ret.insert({"Tap-to-click", tap_default(dev)});
	ret.insert({"Tap-and-drag", drag_default(dev)});
	ret.insert({"Tap-drag-lock", draglock_default(dev)});
	ret.insert({"Left-handed", left_handed_default(dev)});
	ret.insert({"Nat-scrolling", nat_scroll_default(dev)});
	ret.insert({"Middle emulation", middle_emulation_default(dev)});
	str = calibration_default(dev);
	ret.insert({"Calibration", str});
	free(str);

	str = scroll_defaults(dev);
	ret.insert({"Scroll-methodsion", str});
	free(str);

	str = click_defaults(dev);
	ret.insert({"Click-methods", str});
	free(str);

	ret.insert({"Disable-w-typing", dwt_default(dev)});

	str = accel_profiles(dev);
	ret.insert({"Accel-profiles", str});
	free(str);

	str = rotation_default(dev);
	ret.insert({"Rotation", str});
	free(str);

	if (libinput_device_has_capability(dev,
					   LIBINPUT_DEVICE_CAP_TABLET_PAD))
		print_pad_info(dev, ret);

	return ret;
}


/*
static inline void
usage(void)
{
	printf("Usage: libinput list-devices [--help|--version]\n");
	printf("\n"
	       "--help ...... show this help and exit\n"
	       "--version ... show version information and exit\n"
	       "\n");
}
*/

static int
open_restricted(const char *path, int flags, void *user_data)
{
	bool *grab = (bool *)user_data;
	int fd = open(path, flags);

	if (fd < 0)
		fprintf(stderr, "Failed to open %s (%s)\n",
			path, strerror(errno));
	else if (grab && *grab && ioctl(fd, EVIOCGRAB, (void*)1) == -1)
		fprintf(stderr, "Grab requested, but failed for %s (%s)\n",
			path, strerror(errno));

	return fd < 0 ? -errno : fd;
}

static void
close_restricted(int fd, void *user_data)
{
	close(fd);
}

static const struct libinput_interface interface = {
	.open_restricted = open_restricted,
	.close_restricted = close_restricted,
};


static struct libinput *
tools_open_udev(const char *seat, bool verbose, bool *grab)
{
	struct libinput *li;
	struct udev *udev = udev_new();

	if (!udev) {
		fprintf(stderr, "Failed to initialize udev\n");
		return NULL;
	}

	li = libinput_udev_create_context(&interface, grab, udev);
	if (!li) {
		fprintf(stderr, "Failed to initialize context from udev\n");
		goto out;
	}

/*	libinput_log_set_handler(li, log_handler);
	if (verbose)
		libinput_log_set_priority(li, LIBINPUT_LOG_PRIORITY_DEBUG);
*/
	if (libinput_udev_assign_seat(li, seat)) {
		fprintf(stderr, "Failed to set seat\n");
		libinput_unref(li);
		li = NULL;
		goto out;
	}

out:
	udev_unref(udev);
	return li;
}


void iter_list_devices(std::string seat,
		std::function<void(struct libinput_device *dev, const MSS &arg)> func) {
	struct libinput *li;
	struct libinput_event *ev;
	bool grab = false;

	li = tools_open_udev(seat.c_str(), false, &grab);
	if (!li)
		return;

	LMSS ret;
	libinput_dispatch(li);
	while ((ev = libinput_get_event(li))) {

		if (libinput_event_get_type(ev) == LIBINPUT_EVENT_DEVICE_ADDED) {
			struct libinput_device *dev;
			dev = libinput_event_get_device(ev);

			auto i = fill_device_notify(dev);
			func(dev, std::move(i));
		}

		libinput_event_destroy(ev);
		libinput_dispatch(li);
	}

	libinput_unref(li);

}

LMSS get_list_devices(std::string seat) {
	LMSS ret;
	iter_list_devices(std::move(seat),
		[&](struct libinput_device *dev, MSS arg) {
			ret.push_back(std::move(arg));
		}
	);
	return ret;
}

#ifdef DEBUG
#include <iostream>
int main(int argc, char **argv) {
	const auto r = get_list_devices();
	for(const auto &i : r) {
		for(const auto &kv : i)
			std::cout << kv.first << ":\t" << kv.second << "\n";
		std::cout << "\n";
	}

}
#endif
