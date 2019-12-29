#include <iostream>
#include <sstream>

#include "../tasklib/tasklib.h"
#include "../tasklib/util.h"

// Machinery for including the correct library
// Usage: #pragma comment (lib, LIB(some_lib))
#define LIB(name) "..//" ARCH_FOLDER "//" BUILD_FOLDER "//" #name ".lib"
#if defined _WIN64
#define ARCH_FOLDER "x64"
#else
#define ARCH_FOLDER // "x86"
#endif
#if (defined _DEBUG || defined _NDEBUG)
#define BUILD_FOLDER "Debug"
#else
#define BUILD_FOLDER "Release"
#endif

// Link to tasklib to test
#pragma comment (lib, LIB(tasklib))

using namespace std;
using namespace chrono;

template<typename ...T>
void busySleep(const duration<T...>& d) {
	auto start = steady_clock::now();
	while (steady_clock::now() - start < d);
}

void theTask() {
	busySleep(microseconds(1));
}

// generates a random DAG
Workflow makeRandomWorkflow(int numTasks) {
	auto names = vector<string>();
	for (int i = 0; i < numTasks; i++) {
		names.push_back("Task-" + to_string(i));
	}

	auto randomDeps = [&](int max) {
		auto deps = vector<string>();
		if (max > 4) {
			auto count = rand() % (max / 4);
			for (int i = 0; i < count; i++) {
				deps.push_back(names[rand() % max]);
			}
		}
		return deps;
	};

	auto builder = WorkflowBuilder();
	for (int i = 0; i < numTasks; i++) {
		builder.task(names[i], theTask, randomDeps(i - 1));
	}

	return builder.build();
}

int main(int argc, char* argv[]) {
	auto numTasks = (argc > 1) ? parseDouble(argv[1]).value_or(100) : 100;
	auto numThreads = (argc > 2) ? parseDouble(argv[2]).value_or(4) : 4;

	auto wf = makeRandomWorkflow(numTasks);
	auto engine = ConcurrentTaskEngine(8);

	auto testCount = 1000;
	auto totalTime = 0.0;
	auto totalSquareTime = 0.0;
	auto minTime = numeric_limits<double>::max();
	auto maxTime = 0.0;

	for (int i = 0; i < testCount; i++) {
		auto start = steady_clock::now();
		cout << "Starting test" << endl;
		engine.runWorkflow(wf);
		cout << "Ending test" << endl;
		auto end = steady_clock::now();

		auto time = duration_cast<microseconds>(end - start).count() / 1000.0;
		minTime = min(minTime, time);
		maxTime = max(maxTime, time);
		totalTime += time;
		totalSquareTime += time * time;
		cout << "Time taken: " << time << "ms" << endl;
		cout << "Threads: " << numThreads << endl;
		cout << "Tasks:   " << numTasks << endl;
	}

	auto mean = totalTime / testCount;
	auto variance = totalSquareTime / testCount - mean * mean;

	cout << "All tests complete" << endl;
	cout << " - Mean time: " << mean << endl;
	cout << " - Min/Max:   " << minTime << ", " << maxTime << endl;
	cout << " - std. dev.: " << sqrt(variance) << endl;

	getchar();

	return 0;
}