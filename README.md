# Memory Allocator in C

A custom malloc/calloc/realloc/free implementation in C 
using sbrk() system calls directly, with thread safety 
via pthread_mutex_t.
For implementation details, see [**INTERNALS.md**](./INTERNALS.md).

## What this demonstrates
- How heap memory management works at the OS boundary
- Block header metadata and free list traversal
- 16-byte alignment using a union trick
- Why realloc() must never overwrite the original 
  pointer before confirming success
- Basic thread safety with mutex locking

## Build & Run
- `gcc -o memalloc.so -fPIC -shared memalloc.c`
- `export LD_PRELOAD=$PWD/memalloc.so`
- `ls` now uses your malloc.
- Unset using `unset LD_PRELOAD`


## Limitations
- First-fit only, no proactive block coalescing.
- sbrk() is not genuinely thread-safe.
- Not suitable for complex processes (may segfault).
- No optimization for allocation speed.