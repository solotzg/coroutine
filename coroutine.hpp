#ifndef COROUTINE_HPP
#define COROUTINE_HPP

#include <ucontext.h>
#include <functional>
#include <tr1/unordered_map>

class Scheduler;
struct Coroutine;
typedef Coroutine* CoroutinePtr_t;
typedef std::function<void (void)> CoroutineFunc_t;
typedef typename std::tr1::unordered_map<int64_t, CoroutinePtr_t> MapSeqCoroutine_t;
const static int DEFAULT_STACK_SIZE = 64*1024;

enum ECoroutineStatus {
    COROUTINE_READY,
    COROUTINE_RUNNING,
    COROUTINE_DEAD,
    COROUTINE_SUSPEND,
    COROUTINE_CALLING
};

class Coroutine {
public:
    CoroutinePtr_t last_coroutine_ptr_;
    unsigned char* stack_;
    int32_t stack_size_;
    CoroutineFunc_t coroutine_func_;
    ECoroutineStatus status_;
    ucontext_t context_;
    Coroutine(const CoroutineFunc_t & coroutine_func, const int32_t stack_size, int32_t is_main);
    virtual ~Coroutine();
};

class Scheduler {
public:
    static Scheduler * CreateScheduler();
    static void SchedulerFunc(uint64_t ptr);
    int64_t CreateCoroutine(const CoroutineFunc_t & coroutine_func, int32_t is_main = 0, const int32_t stack_size = DEFAULT_STACK_SIZE);
    int32_t Resume(int64_t id);
    int32_t Yield();
    ECoroutineStatus CoroutineStatus(int64_t coroutine_id);

public:
    CoroutinePtr_t running_coroutine() {
        return this->running_coroutine_;
    }
    virtual ~Scheduler();

private:
    MapSeqCoroutine_t map_id_coroutine_;
    int64_t auto_inc_id_;
    CoroutinePtr_t running_coroutine_;
    CoroutinePtr_t main_coroutine_ptr_;

private:
    Scheduler();

};

ECoroutineStatus Scheduler::CoroutineStatus(int64_t coroutine_id) {
    auto it = map_id_coroutine_.find(coroutine_id);
    if (it == map_id_coroutine_.end()) {
        return COROUTINE_DEAD;
    }
    return it->second->status_;
}

int64_t Scheduler::CreateCoroutine(const CoroutineFunc_t & coroutine_func, int32_t is_main, const int32_t stack_size) {
    map_id_coroutine_[auto_inc_id_] = new Coroutine(coroutine_func, stack_size, is_main);
    return auto_inc_id_++;
}

Coroutine::Coroutine(const CoroutineFunc_t & coroutine_func, const int32_t stack_size, int32_t is_main):
    last_coroutine_ptr_(NULL),
    coroutine_func_(coroutine_func),
    status_(COROUTINE_READY) {
    if (is_main) {
        stack_ = NULL;
        stack_size_ = 0;
    } else {
        stack_size_ = stack_size;
        stack_ = (new unsigned char [stack_size_]);
    }
}

Coroutine::~Coroutine() {
    if (stack_ != NULL) {
        delete [] stack_;
        stack_ = NULL;
    }
}

Scheduler* Scheduler::CreateScheduler() {
    return new Scheduler();
}

Scheduler::Scheduler():
    auto_inc_id_(0ll) {
    int64_t main_coroutine_id = Scheduler::CreateCoroutine(CoroutineFunc_t(), 1, 0);
    main_coroutine_ptr_ = map_id_coroutine_[main_coroutine_id];
    main_coroutine_ptr_->status_ = COROUTINE_RUNNING;
    running_coroutine_ = main_coroutine_ptr_;
}

Scheduler::~Scheduler() {
    for (auto it = map_id_coroutine_.begin(); it != map_id_coroutine_.end(); ++it) {
        delete it->second;
        it->second = NULL;
    }
}

void Scheduler::SchedulerFunc(uint64_t ptr) {
    Scheduler * scheduler = (Scheduler *) ptr;
    CoroutinePtr_t running_coroutine = scheduler->running_coroutine();
    running_coroutine->coroutine_func_();
    running_coroutine->status_ = COROUTINE_DEAD;
    swapcontext(&running_coroutine->context_, &running_coroutine->last_coroutine_ptr_->context_);
}

int32_t Scheduler::Resume(int64_t id) {
    MapSeqCoroutine_t::iterator it = map_id_coroutine_.find(id);
    if (it == map_id_coroutine_.end())
        return -1;
    CoroutinePtr_t coroutine_ptr = it->second;
    switch(coroutine_ptr->status_) {
    case COROUTINE_READY:
        getcontext(&coroutine_ptr->context_);
        coroutine_ptr->context_.uc_link = NULL;
        coroutine_ptr->context_.uc_stack.ss_sp = coroutine_ptr->stack_;
        coroutine_ptr->context_.uc_stack.ss_size = coroutine_ptr->stack_size_;
        makecontext(&coroutine_ptr->context_, (void(*)(void))Scheduler::SchedulerFunc, 1, (uint64_t) this);
        break;
    case COROUTINE_SUSPEND:
        break;
    default:
        return -1;
    }
    running_coroutine_->status_ = COROUTINE_CALLING;
    coroutine_ptr->last_coroutine_ptr_ = running_coroutine_;
    running_coroutine_ = coroutine_ptr;
    running_coroutine_->status_ = COROUTINE_RUNNING;
    swapcontext(&running_coroutine_->last_coroutine_ptr_->context_,
                &running_coroutine_->context_);
    coroutine_ptr = running_coroutine_->last_coroutine_ptr_;
    if (running_coroutine_->status_ == COROUTINE_DEAD) {
        delete running_coroutine_;
        map_id_coroutine_.erase(it);
        running_coroutine_ = NULL;
    }
    running_coroutine_ = coroutine_ptr;
    running_coroutine_->status_ = COROUTINE_RUNNING;
    return 1;
}

int32_t Scheduler::Yield() {
    if (running_coroutine_ == main_coroutine_ptr_)
        return -1;
    running_coroutine_->status_ = COROUTINE_SUSPEND;
    swapcontext(&running_coroutine_->context_, &running_coroutine_->last_coroutine_ptr_->context_);
    return 1;
}

#endif // COROUTINE_HPP
