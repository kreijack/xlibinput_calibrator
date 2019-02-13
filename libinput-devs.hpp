#pragma once

#include <string>
#include <list>
#include <array>
#include <vector>

class LibInputDevice {
private:
	std::map<std::string, std::string> info;
	std::string seat;
public:
	LibInputDevice( std::map<std::string, std::string> info_,
			std::string seat_) :
		info(std::move(info_)),
		seat(std::move(seat_))
		{}
	std::array<double,9> get_calibration_matrix() const;
	void set_calibration_matrix(std::array<double,9> cal={1,0,0, 0,1,0, 0,0,1});
	std::vector<std::string> get_capabilities() const;
	std::string get_kernel_name() const
		{ return info.at("Kernel"); }
	std::string get_description() const
		{ return info.at("Device"); }
	std::map<std::string, std::string> data() const { return info; }
	void update_device();
};

std::list<LibInputDevice> find_libinput_device(const std::string &seat,
	const std::vector<std::string> &capabilities={},
	const std::string &description="", const std::string &kernel_name="");
