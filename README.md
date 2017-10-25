# acoro

A simple library that brings you `setjmp`/`longjmp`-based asymmetric coroutines.

## Disclaimer

- *Not thread-safe*
- Header only
- `std::jmp_buf` is supplemented by *AT&T-style* inline assembly for stack
switching
- Not tested on i386 machines, DOS/Windows OSes yet
- Not suitable for implementing generators/iterators yet

## License

MIT
