#include <iostream>

#include "tasklib.h"
#include "thread_safe_log.h"
#include "util.h"

using namespace std;
using namespace chrono;

template<typename ...T>
void busySleep(const duration<T...>& d) {
	auto start = steady_clock::now();
	while (steady_clock::now() - start < d);
}

void theTask() {
	busySleep(microseconds(10));
}

Workflow makeFrameWorkflow() {
	return WorkflowBuilder("Frame")
		.task("Input", theTask)
		.task("Network", theTask)
		.task("Camera", theTask, { "Input", "Network" })
		.task("Transforms", theTask, { "Input", "Network" })
		.task("Sky", theTask, { "Camera" })
		.task("Terrain", theTask, { "Camera" })
		.task("Solids", theTask, { "Camera" })
		.task("Particles", theTask, { "Camera" })
		.task("Dynamics", theTask, { "Camera", "Transforms" })
		.task("Character", theTask, { "Camera", "Transforms" })
		.task("Post", theTask, { "Sky", "Terrain", "Solids", "Particles", "Dynamics", "Character" })
		.task("Gui", theTask, { "Post" })
		.task("ResolveCommandLists", theTask, { "Gui" })
		.build();
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

void test(const Workflow& wf, int threads) {
	LOG("Started submission");
	auto engine = ConcurrentTaskEngine(threads);
	engine.runWorkflow(wf);
	LOG("Completed submission");
}

int main(int argc, char* argv[]) {
	auto numTasks = (argc > 1) ? parseDouble(argv[1]).value_or(100) : 100;
	auto numThreads = (argc > 2) ? parseDouble(argv[2]).value_or(4) : 4;

	auto wf = makeRandomWorkflow(numTasks);

	auto start = steady_clock::now();
	test(wf, numThreads);
	auto end = steady_clock::now();

	cout << "Time taken: " << duration_cast<microseconds>(end - start).count() / 1000.0 << "ms" << endl;
	cout << "Threads: " << numThreads << endl;
	cout << "Tasks:   " << numTasks << endl;
	return 0;
}