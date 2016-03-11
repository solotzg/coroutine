#include "coroutine.hpp"
#include <iostream>

using namespace std;

void func1(Scheduler * scheduler, int num) {
    cout << "running func1" << endl;
    cout << "num is : " << num <<endl;
    cout << "func1 try to yield" <<endl;
    if (scheduler->Yield() == -1) {
        cout<<"error yield func1"<<endl;
    }
    cout << "back to func1" <<endl;
    cout << "end func1" << endl;
}

void func2(Scheduler * scheduler, int func_id) {
    cout << "running func2" << endl;
    scheduler->Resume(func_id);
    cout << "end func2" << endl;
}

void func(Scheduler * scheduler) {
    cout << "running func" << endl;
    int64_t func1_id = scheduler->CreateCoroutine(std::bind(func1, scheduler, 1111));
    scheduler->Resume(func1_id);
    cout << "back to func" <<endl;
    cout << "use func2 to resume func1" <<endl;
    int64_t func2_id = scheduler->CreateCoroutine(std::bind(func2, scheduler, func1_id));
    scheduler->Resume(func2_id);
    cout << "end func" << endl;
}

void test() {
    cout<<"\t begin main func "<<endl<<endl;
    Scheduler * scheduler = Scheduler::CreateScheduler();
    if (scheduler->Yield() != -1)
        cout<<"error yield"<<endl;
    int64_t func_id = scheduler->CreateCoroutine(std::bind(func, scheduler));
    scheduler->Resume(func_id);
    cout<<"\t back to main func "<<endl<<endl;
    int64_t ff = scheduler->CreateCoroutine(std::bind(func1, scheduler, 23333));
    scheduler->Resume(ff);
    scheduler->Resume(ff);
    delete scheduler;
}

int main() {
    test();
    return 0;
}
