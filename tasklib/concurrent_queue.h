#ifndef _CONCURRENT_QUEUE
#define _CONCURRENT_QUEUE

#include <mutex>
#include <condition_variable>
#include <queue>

template<typename Type>
class concurrent_queue {
	mutable std::mutex mtx;
	std::condition_variable cv;
	std::queue<Type> items;

public:
	concurrent_queue() = default;
	concurrent_queue(const concurrent_queue&) = delete;
	concurrent_queue(concurrent_queue&&) = delete; // moving mutexes is a nightmare
	concurrent_queue& operator=(const concurrent_queue&) = delete;
	concurrent_queue& operator=(concurrent_queue&&) = delete;
	~concurrent_queue() = default;

	Type consume() {
		auto l = std::unique_lock(mtx);
		while (items.empty()) {
			cv.wait(l);
		}
		auto i = items.front();
		items.pop();
		return i;
	}
	void produce(const Type& item) {
		auto l = std::scoped_lock(mtx);
		items.push(item);
		cv.notify_one();
	}
};

#endif