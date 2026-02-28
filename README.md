# Memory Allocator in C
This is an implementation of a custom memory allocator in C. It implements the following standard library functions using low-level operating system calls:
- `malloc()`
- `calloc()`
- `realloc()`
- `free()`

## Program Memory Layout
The virtual address space has five sections:
1. Text section: binary instructions to be executed by the processor.
2. Data section: non-zero initialized static data.
3. BSS (Block started by symbol): zero-initialized static data
4. Heap: dynamically allocated data.
5. Stack: automatic variables, function arguments, and base pointers.

The heap and the stack grow in opposite directions. The end of the data segment (Data + BSS + Heap)  is demarcated by a pointer known as the `program break` or `brk`.

## Implementation Details
### Heap Management (`sbrk`)
More memory in the heap is allocated by requesting the system to increment the program break. To release, we simply decrement it. We use `sbrk()` system call to do so.
- `sbrk(0)`: Gives the current address of the program break.
- `sbrk(x)`: Increments the break by `x` bytes (allocating memory).
- `sbrk(-x)`: Decrements the break by `x` bytes (releasing memory).

### Memory Block Structure
To manage memory, we cannot just return raw pointers. We also need to manage metadata about each allocated block. We store the size of the allocated block as well as the status of whether it is free or not. 

We add a header to every allocated memory block, keeping it hidden from the calling program. We use a linked list to keep track of all allocated blocks. The header structure looks like:
```C
struct header_t {
    size_t size;
    unsigned is_free;
    struct header_t *next;
}
```
### Alignment
To ensure proper memory alignment (16-byte boundaries), we wrap the header in a `union`.
```C
typedef char ALIGN[16];
union header {
    struct {
        size_t size;
        unsigned is_free;
        union header *next;
    } s;
    ALIGN stub;
};

typedef union header header_t;
```

### Thread Safety
To prevent two or more threads from concurrently accessing memory, we use a basic locking mechanism with `pthread_mutex_t`.

## Build Instructions

To compile the allocator into a shared object file(`.so`), run:
```bash
gcc -o memalloc.so -fPIC -shared memalloc.c
```
- `-fPIC` generates position-independent code.
- `shared` tells the linker to produce a shared object suitable for dynamic linking.

### Usage Instructions
On Linux, you can use the `LD_PRELOAD` environment variable to force programs to use the custom allocator. 
#### Testing with `ls`
```bash
$ export LD_PRELOAD=$PWD/memalloc.so
$ ls
```
This forces the `ls` command to use these implementations of `malloc()` and `free()`.

#### Unsetting `LD_PRELOAD`
To return to the default system behavior for the current terminal session:
```bash
unset LD_PRELOAD
```
## Limitations
- This is a very basic implementation, and hence can result in `segmentation fault (core dumped)` errors for complex processes.
- This allocator is not optimized for speed.
- It uses a first-fit approach and does not proactively coalesce adjacent free blocks.
- `sbrk`is historical, and not genuinely thread-safeon all platforms, and requires memory to be released in LIFO order.
