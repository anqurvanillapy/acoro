#include <iostream>
#include "acoro.h"

acoro::sched sched;

void
foo(const void *msg)
{
    for (int i = 0; i < 5; ++i) {
        std::cout << (char *)msg << std::endl;
        sched.yield();
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
