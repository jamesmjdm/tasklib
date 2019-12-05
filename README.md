# tasklib

** not quite production-ready! **

Concurrent task queue implementation

Allows to group "tasks" declaratively into a "workflow" with dependency relationships, and run workflow in parallel with a thread pool

#### Example code

Run three functions to calculate 3 intermediate results, then run a final calculation using the 3 intermediate results

The final calculation should not run until all intermediate results are ready

This is achieved by marking the "Final" task as dependent on the other 3

The workflow builder class topologically sorts the subtasks of a workflow, so no task is executed ahead of it's dependencies,

and a running task will wait until all dependencies have completed.

```cpp
#include "tasklib.h"

using namespace std;

int i1 = 0;
int i2 = 0;
int i3 = 0;
int result = 0;

void task1() {
    i1 = 1 * 1;
}
void task2() {
    i2 = 2 * 2;
}
void task3() {
    i3 = 3 * 3;
}
void dependentTask() {
    result = i1 + i2 + i3;
}

int main(int argc, char* argv[]) {
	auto workflow = WorkflowBuilder()
		.task("Task1", task1)
		.task("Task2", task2)
		.task("Task3", task3)
		.task("FinalCalc", task4, { "Task1", "Task2", "Task3" })
		.build();

    auto engine = unique_ptr<TaskEngine>(4);
    engine->runWorkflow(workflow);
    return 0;
}
```

# TODO

- being able to get a result in one task from a previously run task; all tasks could return a future or similar
- being able to get a result from runWorkflow(), could return a future or similar
- better documentation
- testing topological sorting
- testing cycle detection
- recording timings for tasks
- improved logging
- "debug viewer" for a workflow
- queue multiple workflows instead of blocking on runWorkflow
- schedule independent tasks
- schedule workflows and tasks from within a running task
- some kind of parallel map-reduce example?