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

void semaphore::up(int n) {
	auto l = scoped_lock(mtx);
	value += n;
}
void semaphore::down(int n) {
	auto l = scoped_lock(mtx);
	if (value <= n) {
		value = 0;
		cvar.notify_all();
	} else {
		value -= n;
	}
}
void semaphore::wait_for_zero() {
	if (value != 0) {
		auto l = unique_lock(mtx);
		while (value != 0) {
			cvar.wait(l);
			// TODO: lost wakeups?
		}
	}
}