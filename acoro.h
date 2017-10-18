#pragma once

#include <functional>
#include <memory>
#include <list>
#include <csetjmp>
#include <cassert>

#define MAX_ACORO_COUNT 32
#define ACORO_STACKSIZ (1024 * 1024)

namespace acoro {

class acoro_stack_pool {
public:
    acoro_stack_pool()
    {
        for (int i = 0; i < MAX_ACORO_COUNT; ++i) {
            char *s = new char[ACORO_STACKSIZ];
            q_.push_back(s);
        }
    }

    ~acoro_stack_pool()
    {
        for (auto s : q_) delete[] s;
    }

    acoro_stack_pool(const acoro_stack_pool&) = delete;
    acoro_stack_pool& operator=(const acoro_stack_pool&) = delete;
    acoro_stack_pool(acoro_stack_pool&&) = delete;
    acoro_stack_pool& operator=(acoro_stack_pool&&) = delete;

    char *
    acquire()
    {
        if (q_.empty()) return NULL;
        auto ret = q_.front();
        q_.pop_front();
        return ret;
    }

    void
    release(char *s)
    {
        q_.push_back(s);
    }
private:
    std::list<char*> q_;
};

template <typename T, typename... Args>
class acoro {
public:
    acoro(int id, const T& f, Args... args)
    : id_
    {
        auto unbound_func = std::function<T>(f);
        func_ = std::bind(unbound_func, ...args);
    }

    ~acoro() = default;

    acoro(const acoro&) = delete;
    acoro& operator=(const acoro&) = delete;
    acoro(acoro&&) = delete;
    acoro& operator=(acoro&&) = delete;
private:
    std::function<T> func_;
    int id_{};
    std::weak_ptr<char *>stack_{};
    std::jmp_buf env_;
};

// TODO: Wrap into a scheduler class.
static std::list<std::shared_ptr<acoro>> acoro_list_{};
static acoro_stack_pool pool_{};
static int nacoro_{};

std::jmp_buf main_env_;

void
run()
{
}

template <typename T, typename... Args>
void
add()
{
}

void
yield()
{
}

} /* namespace acoro */
