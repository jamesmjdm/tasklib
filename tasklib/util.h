#ifndef _UTIL_H
#define _UTIL_H

#include <optional>
#include <string>
#include <condition_variable>
#include <mutex>
#include <atomic>

// erase-remove idiom for removing from a container type in-place
// shorter to type and easier to remember
// do not call while iterating over the container!!
template<typename Container, typename Type>
void eraseRemove(Container& container, const Type& ty) {
	container.erase(remove(container.begin(), container.end(), ty), container.end());
}

std::optional<double> parseDouble(const std::string& str);

struct semaphore {
	semaphore() = default;
	semaphore(const semaphore&) = delete;
	semaphore& operator=(const semaphore&) = delete;
	semaphore(semaphore&&) = delete;
	semaphore& operator=(semaphore&&) = delete;

	void up(int n = 1);
	void down(int n = 1);
	void wait_for_zero();
	bool is_zero();

private:
	std::atomic<int> value;
	mutable std::mutex mtx;
	std::condition_variable cvar;
};

#endif
