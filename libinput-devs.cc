#include <cassert>
#include <libinput.h>
#include "libinput-list-devices.hpp"
#include "libinput-devs.hpp"


static std::vector<std::string> str_split(const std::string &s) {
	std::vector<std::string> ret;
	int i=0,j=0;
	for ( ; j < (int)s.size() ; j++) {
		if (s[j] == ' ') {
			ret.push_back(s.substr(i, j - i));
			i = j+1;
		}
	}
	if (i != j)
		ret.push_back(s.substr(i, j - i));

	return ret;
}

static std::string str_join(const std::vector<std::string> &src) {
	std::string ret;
	for(const auto &x: src)
		ret += x + " ";
	if (ret.size())
		ret.resize(ret.size()-1);

	return ret;
}

static int vscount(const std::vector<std::string> &v, const std::string &s) {
	int ret = 0;
	for(auto &x: v)
		if (x==s)
			++ret;
	return ret;
}

std::array<double,9> LibInputDevice::get_calibration_matrix() const {
	std::array<double,9> ret;
	const auto cs = str_split(info.at("Calibration"));
	assert(cs.size() == 6);
	for (int i = 0 ; i < 6 ; i++)
		ret[i] = std::stod(cs[i]);
	ret[6] = 0;
	ret[7] = 0;
	ret[8] = 1;

	return ret;
}

void LibInputDevice::set_calibration_matrix(std::array<double,9> cal) {
	assert(cal[6] == 0);
	assert(cal[7] == 0);
	assert(cal[8] == 1);


	std::vector<std::string> v;
	for (int i = 0; i < 6 ; i++)
		v.push_back(std::to_string(cal[i]));
	info["Calibration"] = str_join(v);
}

std::vector<std::string> LibInputDevice::get_capabilities() const {
	return str_split(info.at("Capabilities"));
}

static bool dev_match(const std::map<std::string, std::string> &arg,
		const std::vector<std::string> &capabilities,
		const std::string &description,
		const std::string &kernel_name) {
	if (capabilities.size()) {
		const auto caps = str_split(arg.at("Capabilities"));
		for(const auto &x: capabilities)
			if (vscount(caps, x) == 0)
				return false;
	}

	if (description.size()) {
		if (description != arg.at("Device"))
			return false;
	}

	if (kernel_name.size()) {
		if (kernel_name != arg.at("Kernel"))
			return false;
	}

	return true;
}

std::list<LibInputDevice> find_libinput_device(const std::string &seat,
				const std::vector<std::string> &capabilities,
				const std::string &description,
				const std::string &kernel_name) {

	std::list<LibInputDevice> ret;

	iter_list_devices(seat,
		[&](struct libinput_device *dev,
			const std::map<std::string, std::string> &arg) {

		if (dev_match(arg, capabilities, description, kernel_name))
			ret.push_back(LibInputDevice(arg, seat));
	});

	return ret;
}
#include <cstdio>
void LibInputDevice::update_device() {
	iter_list_devices("seat0",
		[&](struct libinput_device *dev,
			const std::map<std::string, std::string> &arg) {

		if (!dev_match(arg, {}, info["Device"], info["Kernel"]))
			return;

		auto c = get_calibration_matrix();
		float cf[6];
		for (int i = 0 ; i < 6 ; i++) {
			cf[i] = c[i];
			fprintf(stderr, "c[%d]=%f\n",i, cf[i]);
		}

		if (libinput_device_config_calibration_has_matrix(dev)) {
			fprintf(stderr, "The device haven't a calibration matrix\n");
		}
		auto ret = libinput_device_config_calibration_set_matrix(dev, cf);
		switch (ret) {
			case LIBINPUT_CONFIG_STATUS_SUCCESS:
				fprintf(stderr, "Success\n");
				break;
			case LIBINPUT_CONFIG_STATUS_UNSUPPORTED:
				fprintf(stderr, "LIBINPUT_CONFIG_STATUS_UNSUPPORTED\n");
				break;
			case LIBINPUT_CONFIG_STATUS_INVALID:
				fprintf(stderr, "LIBINPUT_CONFIG_STATUS_INVALID\n");
				break;
			default:
				fprintf(stderr, "Boh???\n");
				break;
		}
		auto ret2 = libinput_device_config_calibration_get_matrix(dev, cf);
		switch (ret2) {
			case LIBINPUT_CONFIG_STATUS_SUCCESS:
				fprintf(stderr, "Success\n");
				break;
			case LIBINPUT_CONFIG_STATUS_UNSUPPORTED:
				fprintf(stderr, "LIBINPUT_CONFIG_STATUS_UNSUPPORTED\n");
				break;
			case LIBINPUT_CONFIG_STATUS_INVALID:
				fprintf(stderr, "LIBINPUT_CONFIG_STATUS_INVALID\n");
				break;
			default:
				fprintf(stderr, "Boh???\n");
				break;
		}
		for (int i = 0 ; i < 6 ; i++)
			fprintf(stderr, "%f ", cf[i]);
		fprintf(stderr, "\n");

	});
}


#ifdef DEBUG
#include <cassert>
#include <cstdio>
#include <iostream>

int main() {
	auto s = str_split("abc def");
	assert(vscount(s, "abc"));
	assert(vscount(s, "def"));
	assert(s.size()==2);

	auto s1 = str_join({"abc", "def"});
	assert(s1.find("abc") != std::string::npos);
	assert(s1.find("def") != std::string::npos);
	assert(s1.size()==7);

	auto l1 = find_libinput_device("seat0", {"touch"});
	assert(l1.size() == 1);

	auto d = *l1.begin();
	fprintf(stderr,"Cal matrix(%s, %s):",
		d.get_description().c_str(), d.get_kernel_name().c_str());
	for(auto x : d.get_calibration_matrix())
		fprintf(stderr,"%f ", x);
	fprintf(stderr,"\n");

	d.set_calibration_matrix({1,2,3,4,5,6,0,0,1});
	fprintf(stderr,"Cal matrix(%s, %s):",
		d.get_description().c_str(), d.get_kernel_name().c_str());
	for(auto x : d.get_calibration_matrix())
		fprintf(stderr,"%f ", x);
	fprintf(stderr,"\n");
	d.update_device();

	l1 = find_libinput_device("seat0", {"touch"});
	assert(l1.size() == 1);
	d = *l1.begin();
	fprintf(stderr,"Cal matrix(%s, %s):",
		d.get_description().c_str(), d.get_kernel_name().c_str());
	for(auto x : d.get_calibration_matrix())
		fprintf(stderr,"%f ", x);
	fprintf(stderr,"\n");
	return 0;
}
#endif
