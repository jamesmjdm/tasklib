#ifndef _THREAD_SAFE_LOG_H
#define _THREAD_SAFE_LOG_H

// Basic atomic logging

#include <iostream>
#include <mutex>
#include <string>
#include <sstream>

template<typename First>
void log_entry(std::ostream& o, First first) {
	o << first;
}

template<typename First, typename... Args>
void log_entry(std::ostream& o, First first, Args... args) {
	o << first;
	log_entry(o, args...);
}

template<typename ... Args>
void LOG(Args... args) {
	static auto m = std::mutex(); // shared by all threads

	// buffer up the log entry
	auto s = std::stringstream();
	log_entry(s, args...);

	// push log entry out
	auto lock = std::scoped_lock(m);
	std::cout << s.str() << std::endl;
}

#endif
