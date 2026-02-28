// unistd.h = UNIX standard header. Provides access to low-level OS services in C
// Functions declared here are usually system calls or wrappers around them
// sbrk(), read(), (write), etc,.
#include <unistd.h>

// string.h provides string and memory manipulation functions like memcpy(), memset(), memcmp()
#include <string.h>

// provides POSIX threads functions
// Required for thread safety
// Gives pthread_mutex_lock, pthread_mutex_unlock
#include <pthread.h>
# include <stdio.h> // Just for debug printf

// typedef char ALIGN[16];
typedef long double ALIGN

union header {
    struct {
        size_t size;
        unsigned is_free;
        union header *next;
    } s;
    // Force the header to be aligned to 16 bytes
    // Union in C dictates that all its members share the same memory location.
    // Its total size and alignment requirement must be at least as large as the largest and most strictly aligned member within it.
    ALIGN stub;
};

typedef union header header_t;

header_t *head = NULL, *tail = NULL;
pthread_mutex_t global_malloc_lock; // Mutex lock


// Function to get free block if available from already allocated ones
header_t* get_free_block(size_t size) {
    header_t *curr = head;
    while (curr) {
        // Identifying if there is a free block with enough size to accomodate requested size
        if (curr->s.is_free && curr->s.size >= size) {
            return curr;
        }
        curr = curr->s.next;
    }
    return NULL;
}

