#include <iostream>
#include "acoro.h"

acoro::sched sched;

void
foo(const void* msg)
{
    for (int i = 0; i < 5; ++i) {
        std::cout << (char*)msg << "\n";
        sched.yield();
        printf("\tcan i reach here? i=%d\n", i);
    }
}

int
main()
{
    sched.add(foo, "hello");
    sched.add(foo, "world");
    assert(!sched.full());
    sched.run();
    return 0;
}
