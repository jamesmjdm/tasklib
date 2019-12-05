#include "tasklib.h"
#include "thread_safe_log.h"
#include "util.h"

using namespace std;
using namespace chrono;

WorkflowBuilder::WorkflowBuilder(const string& _name): workflowName(_name) {}

WorkflowBuilder& WorkflowBuilder::task(const string& name, const TaskFunction& func) {
	if (tasks.find(name) != tasks.end()) {
		throw runtime_error("Task by the name " + name + " already exists");
	}
	auto& task = tasks[name]; // map::[] creates a new one if not existing
	task.name = name;
	task.func = func;
	task.dependencies = {};
	return *this;
}
WorkflowBuilder& WorkflowBuilder::task(const string& name, const TaskFunction& func, const vector<string>& depends) {
	task(name, func);
	// TODO: go through dependencies
	tasks[name].dependencies = depends;
	return *this;
}
Workflow WorkflowBuilder::build() {
	auto wf = vector<Workflow::Task>(); // tasks topologically sorted
	
	/*
		// adapted from wiki topological sort: https://en.wikipedia.org/wiki/Topological_sorting
		L = Empty list that will contain the sorted elements
		S = Set of all nodes with no incoming edge // IE no dependencies
		while S is non-empty do
			remove a node n from S
			add n to tail of L
			for each node M with an edge E from N to M do
				remove edge E from the graph
				if M has no other incoming edges then
					insert M into S
		if graph has edges then
			return error   (graph has at least one cycle)
		else
			return L   (a topologically sorted order)
	*/
	
	// construct dependency graphs
	auto roots = vector<Task*>(); // list of nodes with no dependencies - these are executed first
	auto dependencies = unordered_map<Task*, vector<Task*>>();// dependency graph
	auto reverseDeps = unordered_map<Task*, vector<Task*>>(); // reverse dependency graph
	for (auto& t : tasks) {
		auto pt = &(t.second);
		auto numDependencies = pt->dependencies.size();
		if (numDependencies == 0) {
			roots.push_back(pt);
		}
		else {
			dependencies[pt].reserve(numDependencies);
			for_each(pt->dependencies.begin(), pt->dependencies.end(), [&,this](auto& dep) {
				auto pd = &tasks[dep];
				dependencies[pt].push_back(pd);
				reverseDeps[pd].push_back(pt);
			});
		}
	}

	// do the topological sort
	while (roots.size()) {
		auto n = roots.back(); roots.pop_back();
		wf.push_back({ n->name, n->func, {} });
		// for each node M of N reverseDeps
		for (auto& m : reverseDeps[n]) {
			auto& mdeps = dependencies[m];
			eraseRemove(mdeps, n);
			if (mdeps.size() == 0) {
				// m has no more unaccounted dependencies,
				// => m is a new "root" task
				roots.push_back(m);
			}
		}
	}

	// check for cycles
	// if there are any dependencies left, graph has at least one cycle
	auto cycles = stringstream();
	bool foundCycles = false;
	for (const auto& [t, v] : dependencies) {
		if (v.size()) {
			cycles << t->name << '\n';
		}
	}
	if (foundCycles) {
		throw runtime_error("Found cycles - check nodes: " + cycles.str());
	}

	// populate the dependencies now that all indices are fixed
	for (auto& wft : wf) {
		auto task = tasks[wft.name];
		for (auto& depname : task.dependencies) {
			auto dep = find_if(wf.begin(), wf.end(), [&](auto t) { return t.name == depname; });
			if (dep == wf.end()) {
				// this should never happen!
				throw runtime_error("CRITICAL ERROR: Could not find task '" + depname + "' while resolving dependencies");
			}
			auto index = distance(wf.begin(), dep);
			wft.dependencies.push_back(index);
		}
	}

	// wftasks no longer used, we can move it
	return Workflow(workflowName, move(wf));
}

Workflow::Workflow(const string& _name, vector<Workflow::Task>&& _subtasks): name(_name), subtasks(_subtasks) {}
const Workflow::task_store& Workflow::getSubtasks() const {
	return subtasks;
}


ConcurrentTaskEngine::Task::Task(const string& _name, const TaskFunction& _func, vector<ConcurrentTaskEngine::Task*>&& _dependencies) :
	name(_name),
	func(_func),
	dependencies(_dependencies) {}
ConcurrentTaskEngine::Task::Task(ConcurrentTaskEngine::Task&& t) :
	name(move(t.name)),
	func(move(t.func)),
	dependencies(move(t.dependencies)) {
		complete = t.complete.load();
		// TODO: is this dodgy?
}
void ConcurrentTaskEngine::Task::waitUntilComplete() {
	if (!complete) {
		auto l = unique_lock(completeMtx);
		while (!complete) {
			completeVar.wait(l);
		}
		l.unlock();
	}
}
void ConcurrentTaskEngine::Task::run() {
	func();
	complete = true;
	completeVar.notify_all();
}


ConcurrentTaskEngine::ConcurrentTaskEngine(int numWorkers) {
	auto workerMain = [&, this](int workerId) {
		while (true) {
			auto task = taskQueue.consume();
			if (task) {
				LOG("Worker: ", workerId, " received task: ", task->name);
				for (auto d : task->dependencies) {
					LOG("Worker: ", workerId, " waiting for dependency ", d->name);
					d->waitUntilComplete();
				}
				LOG("Worker: ", workerId, " running task: ", task->name);
				task->run();
			} else {
				// when the engine is destroyed, "nullptr" task is sent to all engines
				// workers should exit gracefully when they see this
				LOG("Worker: ", workerId, " received kill-task; exiting...");
				break;
			}
		}
	};

	LOG("Spawning ", numWorkers, " workers");
	for (int i = 0; i < numWorkers; i++) {
		workers.push_back(thread(workerMain, i));
	}
}

ConcurrentTaskEngine::~ConcurrentTaskEngine() {
	// send "kill" messages - bit of a hack...
	LOG("Sending 'kill' message to workers...");
	for (auto& w : workers) {
		taskQueue.produce(nullptr);
	}
	
	// join all workers
	LOG("Waiting for workers to join...");
	for (auto& w : workers) {
		w.join();
	}

	LOG("All workers terminated");
}

void ConcurrentTaskEngine::runWorkflow(const Workflow& w) {
	// entire method atomic
	// MUST BLOCK until tasks complete...
	auto l = scoped_lock(runWorkflowMtx);

	// copy workflow tasks to temporary backing store, fix dependencies as pointers
	auto backlog = vector<Task>(w.getSubtasks().size());
	for (auto& t : w.getSubtasks()) {
		auto deps = vector<Task*>();
		for (auto d : t.dependencies) {
			// dependencies must already be in the backlog due to sorting, so this is safe
			deps.push_back(&backlog[d]);
		}
		backlog.emplace_back(t.name, t.func, move(deps));
	}

	// push tasks to work queue
	for (auto& t : backlog) {
		taskQueue.produce(&t);
	}
	
	// worker threads have now started processing tasks
	// block the current thread until all tasks complete
	// TODO: this is inefficient
	for (auto& t : backlog) {
		t.waitUntilComplete();
	}
}