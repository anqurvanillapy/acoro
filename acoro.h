#pragma once

#include <functional>
#include <memory>
#include <list>
#include <csetjmp>
#include <cassert>

#define MAX_ACORO_COUNT 32
#define ACORO_STACKSIZ (1024 * 1024)

namespace acoro {

class stack_pool {
public:
    stack_pool()
    {
        for (int i = 0; i < MAX_ACORO_COUNT; ++i) {
            char *s = new char[ACORO_STACKSIZ];
            q_.push_back(s);
        }
    }

    ~stack_pool() { for (auto s : q_) delete[] s; }

    stack_pool(const stack_pool&)               = delete;
    stack_pool& operator=(const stack_pool&)    = delete;
    stack_pool(stack_pool&&)                    = delete;
    stack_pool& operator=(stack_pool&&)         = delete;

    char *
    acquire()
    {
        if (q_.empty()) return NULL;
        auto ret = q_.front();
        q_.pop_front();
        return ret;
    }

    void release(char *s) { q_.push_back(s); }
private:
    std::list<char*> q_;
};

template <typename T>
class asfn {
public:
    asfn(int id, const std::function<T>& f) : id_{id}, func_{f} { /* nop */ }
    ~asfn() = default;

    asfn(const asfn&)               = delete;
    asfn& operator=(const asfn&)    = delete;
    asfn(asfn&&)                    = delete;
    asfn& operator=(asfn&&)         = delete;
private:
    std::function<T> func_;
    int id_{};
    std::weak_ptr<char *>stack_{};
    std::jmp_buf env_;
};

template <typename T, typename ...Args>
class sched {
public:
    sched() = default;
    ~sched() = default;

    sched(const sched&)             = delete;
    sched& operator=(const sched&)  = delete;
    sched(sched&&)                  = delete;
    sched& operator=(sched&&)       = delete;

    bool full() { return nacoro_ > MAX_ACORO_COUNT; }

    void
    add(int id, const T& f, Args... args)
    {
        assert(0 <= nacoro_ && nacoro_ <= MAX_ACORO_COUNT);
        if (nacoro_ + 1 > MAX_ACORO_COUNT) return;
        auto fntor = std::bind(f, args...);
        auto new_asfn = std::make_shared<asfn<T>>(id, std::move(fntor));
        acoro_list_.emplace_back(std::move(new_asfn));
    }

    void
    run()
    {
    }

    void
    yield()
    {
    }
private:
    static std::list<std::shared_ptr<asfn<T>>> acoro_list_;
    static stack_pool pool_;
    static int nacoro_;
    static std::jmp_buf main_env_;
};

template <typename T, typename ...Args>
std::list<std::shared_ptr<asfn<T>>> sched<T, Args...>::acoro_list_{};
template <typename T, typename ...Args>
stack_pool sched<T, Args...>::pool_{};
template <typename T, typename ...Args>
int sched<T, Args...>::nacoro_{};

} /* namespace acoro */
