#pragma once

#include <functional>
#include <array>
#include <csetjmp>
#include <cassert>

#define MAX_ACORO_COUNT 32
#define ACORO_STACKSIZ (1024 * 1024)

class acoro_stack {
public:
    acoro_stack()
    {
        for (int i = 0; i < MAX_ACORO_COUNT; ++i) {
            stack_list_[i] = new char[ACORO_STACKSIZ];
            assert(stack_list_[i]);
        }
    }
    ~acoro_stack()
    {
        for (int i = 0; i < MAX_ACORO_COUNT; ++i) delete[] stack_list_[i];
    }
private:
    std::array<char*, MAX_ACORO_COUNT> stack_list_{};
    int enqueue_pos_{}, dequeue_pos_{};
};

template <typename T>
class acoro {
public:
    acoro() = default;
    ~acoro() = default;
private:
    std::function<T> func_;
    int id_;
    void *stack_;
    std::jmp_buf env_;
};
