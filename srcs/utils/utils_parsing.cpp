#include "../../include/webserv.h"

bool isEmpty(std::string str) {
	str.erase(0, str.find_first_not_of(" \t\n\r"));
	return str.empty();
}
