#ifndef _TASKLIB_H
#define _TASKLIB_H

#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <functional>
#include <atomic>
#include <algorithm>
#include <unordered_map>

#include "concurrent_queue.h"
#include "util.h"

using TaskFunction = std::function<void()>;
using uint = unsigned int;

class Workflow {
	friend class WorkflowBuilder; // because Task is private, effectively WorkflowBuilder is the only way to create Workflow
	
	struct Task {
		std::string name;
		TaskFunction func;
		std::vector<uint> dependencies; // index of dependencies in subtask list
	};
	using task_store = std::vector<Task>;

	const std::string name;
	const task_store subtasks; // list of tasks, in a topological order

public:
	Workflow() = default;
	Workflow(const std::string& _name, std::vector<Task>&& _subtasks);
	Workflow(const Workflow&) = default;
	Workflow& operator=(const Workflow&) = default;
	Workflow(Workflow&&) = default;
	Workflow& operator=(Workflow&&) = default;
	~Workflow() = default;

	// thou shalt not modify subtasks without a WorkflowBuilder
	const task_store& getSubtasks() const;
};


struct WorkflowBuilder {
private:
	struct Task {
		std::string name;
		TaskFunction func;
		std::vector<std::string> dependencies; // names of dependencies
	};

	std::string workflowName;
	std::unordered_map<std::string, WorkflowBuilder::Task> tasks;

public:
	WorkflowBuilder() = default;
	WorkflowBuilder(const std::string& _name);
	WorkflowBuilder(const WorkflowBuilder&) = default;
	WorkflowBuilder& operator=(const WorkflowBuilder&) = default;
	WorkflowBuilder(WorkflowBuilder&&) = default;
	WorkflowBuilder& operator=(WorkflowBuilder&&) = default;
	~WorkflowBuilder() = default;

	// a task with no dependencies
	WorkflowBuilder& task(const std::string& name, const TaskFunction& func);
	// a task with some dependencies
	WorkflowBuilder& task(const std::string& name, const TaskFunction& func, const std::vector<std::string>& depends);
	// a task that depends on all tasks (that have been defined SO FAR)
	WorkflowBuilder& taskFinal(const std::string& name, const TaskFunction& func);
	Workflow build();
};

class TaskEngine {
public:
	enum {
		DO_NOT_BLOCK = 1
	} Flag;
	virtual ~TaskEngine() {}
	virtual void runWorkflow(const Workflow& wf, unsigned int flags=0) = 0;
};

class ConcurrentTaskEngine : public TaskEngine {
protected:
	struct Task {
		// const obviates locking?
		const std::string name;
		const TaskFunction func;
		const std::vector<Task*> dependencies; // points to members of the backlog
		
		mutable std::mutex completeMtx;
		std::condition_variable completeVar;
		std::atomic<bool> complete = false;

		Task() = default;
		Task(const std::string& _name, const TaskFunction& _func, std::vector<Task*>&& _dependencies);
		Task(const Task&) = delete;
		Task& operator=(const Task&) = delete;
		Task(Task&&); // so we can use it in std::vector
		Task& operator=(Task&&) = delete;
		~Task() = default;
		
		void waitUntilComplete();
		void run();
	};

	// worker threads
	std::vector<std::thread> workers;
	// the actual shared queue; points to members of backlog
	concurrent_queue<Task*> taskQueue;

	// make sure that workflows are queued atomically
	mutable std::mutex backlogMtx;
	std::vector<Task> backlog;
	semaphore backlogCount;

	// called by each worker when they complete a task
	// if that was the last task in the backlog, will notify all threads
	// that are blocked on waitBacklog
	void workerCompletedTask();

public:
	ConcurrentTaskEngine(int numWorkers);
	ConcurrentTaskEngine(const ConcurrentTaskEngine&) = delete;
	ConcurrentTaskEngine(ConcurrentTaskEngine&&) = delete;
	virtual ~ConcurrentTaskEngine();
	ConcurrentTaskEngine& operator=(const ConcurrentTaskEngine&) = delete;
	ConcurrentTaskEngine& operator=(ConcurrentTaskEngine&&) = delete;

	// run a workflow
	// will block current thread until complete
	// flags doesn't do anything yet
	virtual void runWorkflow(const Workflow& wf, unsigned int flags) override;
	// block until current workflow to complete
	void waitForBacklog();
	// check if backlog is complete
	bool isBacklogComplete();
};

#endif
