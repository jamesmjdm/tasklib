# tasklib

## Basic implementation of a multithreaded task runner

## Usage

```cpp
#include "tasklib.h"

using namespace std;

int i1 = 0;
int i2 = 0;
int i3 = 0;
int result = 0;

int main(int argc, char* argv[]) {
	auto workflow = WorkflowBuilder()
		.task("Task1", task1)
		.task("Task2", task2)
		.task("Task3", task3)
		.task("Task4", task4)
		.build();


}
```