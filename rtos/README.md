/rtos
================

__Core RTOS functionality.__

I have tried to limit the system calls and services provided to more or less the bare minimum
needed to implement real-life systems.

The only traditional synchronization primitives provided are __counting semaphores__, and non-standard low-level
events that can wake up a task if an event happens in the future.

There are ways for a task to sleep a certain amount of time also __critical sections__ are supported.

There are also a number of API calls to create tasks, stop tasks, change task priorities.

Scheduling is priority based, but time slices are supported (preempt a task when its time slice has expired).

Official Website: http://jaeos.com/

