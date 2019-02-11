#pragma once

#include <list>
#include <string>
#include <map>

std::list<std::map<std::string, std::string>> get_list_devices(std::string seat="seat0");
