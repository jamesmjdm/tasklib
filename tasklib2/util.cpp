#include "util.h"

using namespace std;

optional<double> parseDouble(const string& str) {
	auto begin = str.c_str();
	auto ptr = (char*)nullptr;
	auto val = strtod(begin, &ptr);
	if (ptr == begin) {
		return std::nullopt;
	}
	return std::optional(val);
}