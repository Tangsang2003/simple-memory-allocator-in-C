// unistd.h = UNIX standard header. Provides access to low-level OS services in
// C
// Functions declared here are usually system calls or wrappers around them
// sbrk(), read(), (write), etc,.
#include <unistd.h>

// string.h provides string and memory manipulation functions like memcpy(),
// memset(), memcmp()
#include <string.h>

// provides POSIX threads functions
// Required for thread safety
// Gives pthread_mutex_lock, pthread_mutex_unlock
#include <pthread.h>
#include <stdio.h> // Just for debug printf

typedef char ALIGN[16];
// typedef long double ALIGN;

union header
{
    struct
    {
        size_t size;
        unsigned is_free;
        union header *next;
    } s;
    // Force the header to be aligned to 16 bytes
    // Union in C dictates that all its members share the same memory location.
    // Its total size and alignment requirement must be at least as large as the
    //  largest and most strictly aligned member within it.
    ALIGN stub;
};

typedef union header header_t;

header_t *head = NULL, *tail = NULL;
pthread_mutex_t global_malloc_lock; // Mutex lock

// Function to get free block if available from already allocated ones
header_t *get_free_block(size_t size)
{
    header_t *curr = head;
    while (curr)
    {
        // Identifying if there is a free block with enough size to accomodate
        // requested size
        if (curr->s.is_free && curr->s.size >= size)
        {
            return curr;
        }
        curr = curr->s.next;
    }
    return NULL;
}

// Implementation of free()
void free(void *block)
{
    header_t *header, *tmp;
    void *program_break;

    if (!block)
    {
        return;
    }
    pthread_mutex_lock(&global_malloc_lock); // Mutex lock
    header = (header_t *)block - 1;

    // program_break is the end address of the process's data segment
    // sbrk(0) gives the current program break address
    program_break = sbrk(0);

    /* Check if the block to be freedis the last one in the list. If yes, then
    simply shrink the size of the heap and release memory to OS. Else, keep
    the block but mark as free. */
    /* (char*)blocktakes the pointer to the start of the block and cast to char*
    which allows pointer arithmetic in bytes*/
    if ((char *)block + header->s.size == program_break)
    {
        if (head == tail)
        {
            head = tail = NULL;
        }
        else
        {
            tmp = head;
            while (tmp)
            {
                if (tmp->s.next == tail)
                {
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }

        /*
        sbrk() with negative argument decrements the program break. So memory is
        released by the program to the OS.
        */
        sbrk(0 - header->s.size - sizeof(header_t));
        /*
        The lock below isn't necessarily thread safe as sbrk() itself is not really
        thread safe. Suppose there occurs a foreign sbrk(N) after we find the
        program breakand before we decrement it, then we end up releasing the
        memory obtained by the foreign sbrk().
        */
        pthread_mutex_unlock(&global_malloc_lock);
        return;
    }
    header->s.is_free = 1;
    pthread_mutex_unlock(&global_malloc_lock);
}

void *malloc(size_t size)
{
    size_t total_size;
    void *block;
    header_t *header;

    if (!size)
    {
        return NULL;
    }
    pthread_mutex_lock(&global_malloc_lock);
    header = get_free_block(size);
    if (header)
    {
        // Free block to accomodate requested memory.
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_lock);
        return (void *)(header + 1);
    }

    // Request memory to fit in the request block and header from OS.
    total_size = sizeof(header_t) + size;
    block = sbrk(total_size);
    if (block == (void *)-1)
    { // sbrk returns (void*) -1 on failure.
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }

    header = block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;

    if (!head)
    {
        head = header;
    }

    if (tail)
    {
        tail->s.next = header;
    }
    tail = header;
    pthread_mutex_unlock(&global_malloc_lock);
    return (void *)(header + 1);
}

void *calloc(size_t num, size_t nsize)
{
    size_t size;
    void *block;
    if (!num || !nsize)
    {
        return NULL;
    }
    size = num * nsize;
    // Check multiplication overflow
    if (nsize != size / num)
    {
        return NULL;
    }
    block = malloc(size);
    if (!block)
    {
        return NULL;
    }
    memset(block, 0, size); // Set the allocated memory to 0
    return block;
}

void *realloc(void *block, size_t size)
{
    header_t *header;
    void *ret;
    if (!block || !size)
    {
        return malloc(size);
    }
    header = (header_t *)block - 1;
    if (header->s.size >= size)
    {
        return block; // if block size is greater than required, do nothing
    }
    ret = malloc(size);
    if (ret)
    {
        // Relocate contents to the new bigger block
        memcpy(ret, block, header->s.size); // header->s.size the number of
        // bytes to copy
        free(block);
    }
    return ret;
}

// Debug function to print the entire linked list
void print_mem_list()
{
    header_t *curr = head;
    printf("head = %p, tail = %p \n", (void *)head, (void *)tail);
    while (curr)
    {
        printf("addr = %p, size = %zu, is_free=%u, next=%p\n",
               (void *)curr, curr->s.size, curr->s.is_free, (void *)curr->s.next);
        curr = curr->s.next;
    }
}
