#pragma once

#include <list>
#include <string>
#include <map>
#include <functional>

std::list<std::map<std::string, std::string>> get_list_devices(std::string seat="seat0");
void iter_list_devices(std::string seat,
	std::function<void(struct libinput_device *dev,
		const std::map<std::string, std::string> &arg)> func);
