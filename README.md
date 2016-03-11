# coroutine

* It's a simple coroutine library for C++
* Two classes are provided: Scheduler & Coroutine
* There are 5 statuses for a Coroutine: READY, RUNNING , DEAD, SUSPEND, CALLING
* Every time you create a coroutine, you can get it's id which is 16 bytes long. 
* You can resume or yield a coroutine by using it's id to call the functions provided by scheduler
 
### Notice
* Never resume a coroutine which is waiting for other coroutine to yield.

### Requirement
* g++
* c++0x
