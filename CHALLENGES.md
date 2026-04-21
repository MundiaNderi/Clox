# Our reallocate() function relies on the C standard library for dynamic memory allocation and freeing. malloc() and free() aren’t magic. Find a couple of open source implementations of them and explain how they work. How do they keep track of which bytes are allocated and which are free? What is required to allocate a block of memory? Free it? How do they make that efficient? What do they do about fragmentation?

See [Dan Luu's malloc tutorial](https://danluu.com/malloc-tutorial/)
See [Malloc](https://cs341.cs.illinois.edu/coursebook/Malloc)
See [Bins and Chunks](https://heap-exploitation.dhavalkapil.com/diving_into_glibc_heap/bins_chunks)
See [Understanding glibc malloc](https://sploitfun.wordpress.com/2015/02/10/understanding-glibc-malloc/)
See [Understanding the Glibc Heap Implementation](https://azeria-labs.com/heap-exploitation-part-2-glibc-heap-free-bins/)
See [Linux man page](https://linux.die.net/man/3/malloc)
See [How tmalloc works](https://www.jamesgolick.com/2013/5/19/how-tcmalloc-works.html)

The OS gives the program a heap(also known as the data segment), starting from a known address, with a pointer called the memory break, marking where usable memory ends.

- You can move that break upwards with the sbrk() system call or mmap() for large regions to get more memory from the kernel
- The heap is a contigous block of memory that the program can expand or contract. Most programs don't interact with sbrk() directly, they use an allocator on top of it that chucks out memory and and keeps track of what's allocated and what's free

The allocator has to:

- Carve the heap into independent reusable pieces
- Remember which pieces are free.
- Hand out a piece that curves any given request
- Accept pieces back and eventually reuse them

## A simple allocator

Each block of allocated memory contains two sections: a metadata block storing meta information and a metadata itself. The structure of the metadata looks as below:

```
struct block_meta {
    size_t size;       // how big is the data region
    int    free;       // is it available?
    struct block_meta *next;  // linked list pointer
};

```

A common trick is to store meta information about a memory region in a space squirreled away just below the pointer returned to the caller.

Say the heap top is at 0x1000, and you ask for 0x400 bytes, intead of requesting exactly 0x400 bytes from `sbrk`, you request 0x410 and return a pointer to 0x1010, hiding the 0x10 bytes of metadata from the calling code.

![Malloc metadata layout](/images/malloc_hidden_metadata.svg)

To allocate:

- You traverse the linked list looking for a block with at least the required size.
- If an exact fit block exists, you remove it from the list and return its address.
- if the block is larger, split it into two, return the one with the requested size and add the remainder back to the list
- if no block fits, call `sbrk` to get more memory from the OS

To free:

- Given the pointer previously returned to the user,find it's metadata using a BLOCK_HEADER macro(pointer arithmetic to reach the header sitting just before the data), then add it back to the free linked list

### The fragmentation problem

- After many alloc/free calls with many sizes, you accumulate many small free blocks that are useless for large requests.
  - A scanAndCoalesce() traverses the free list looking for contiguous free blocks - pairs of adjacent regions, and merges them into a single large block.
  - After coalescing, if the last block on the list ends at the current program break, brk() is called to shrink the heap and return memory to the OS.

- This works but it has two painful properties: finding a free block is O(n) in the number of free blocks, and every search traverses the whole list

# Implementation 2: glibc's ptmalloc2 (the real thing on Linux)

glibc's alocator is a descendant of dmalloc(Doug Lea's malloc), forked into ptmalloc which added thread support. This is what runs when you call `malloc` on Linux

dmalloc is a boundary tag allocator:

- Memory is allocated as chunks: 8 byte aligned structures containing a header and usable memory
- Allocated chunks have an 8 or 16 byte overhead storing the chunk size usage flags.
- Unallocated chunks additionally store pointers to other free chunks in what would be the usable sapce, making the minimum chunk size 16 bytes on 32 bits and 24-32 bytes on 64 bits.
- Free chunks store size information both before and after the chunk: boundary tags at both ends. This allows the heap to be traversed from any known chunk, enabling very fast coalescing of adjacent free chunks

## The bin system

Instead of one linked list, glibc maintains many sorted lists of free chunks(roughly ≤ 80 bytes on 64-bit). No coalescing happens here, chunks go back straight to the bin.
Small bins:

- For sizes 16-504 bytes, each holding chunks of exactly one size in a doubly-linked list with FIFO ordering.

Large bins

- 63 bins covering sizes above 504 bytes, each bin covers a range of sizes(the first 32 bins pan 64-byte ranges, then 512 byte ranges etc), sorted in descending order within each bin

Unsorted bins:

- A single staging area where non-fast chunks are freed, they go here first, giving them a chance to be reused immediately without the overhead of sorting

The allocation path roughly goes:

- check tcache(per-thread cache, modern glibc) -> check small bin ->check unsorted bin(and sort its contents into small/large bins) ->check large bin ->split the "top" chunk(end of the heap) ->call `sbrk/mmap` for more
- For allocations larger than MMAP_THRESHOLD(128 KB by default), glibc skips the heap entirely and uses the `mmap` to allocate a private anonymous mapping

### Fastbins and deferred coalescing

Fastbins are a deliberate trade-off. No coalescing happens when chunks go into a fast bin: two adjacent free chunks can sit next to each other without being merged. This avoids the overhead of coalescing during free, but it means fragmentation occurs over time.

To resolve this, the heap manager periodically consolidates the heap: it flushes each fastbin by doing a proper free, merging entries with adjacent chunks and placing results into the unsorted bin.

- Consolidation happens when a malloc request is made too large for any fastbin to service , when freeing any chunk over 64KB, or when mmaloc_trim or mallopt are called.

### Thread safety: arenas

To avoid corruption in multithreaded applications, mutexes protect the internal data structures.
To scale across many threads, glibc creates additional memory allocation arenas when mutex contention is detected. Each arena is a large region of memory managed with it own mutexes

# Google's tcmalloc

TCMalloc(Thread-Caching Malloc) takes the thread-locality idea further

Like most mordern allocators, TCMalloc is page oriented: the internal unit of measure is usually pages rather than bytes. This makes fragmentation reduction and metadata tracking even simpler.

- tcmalloc defines a page as 8192 bytes.
- small chunks are further divided into size classes and serviced by thread caches.
- large chunks are always satsfied by the central PageHeap

In per thread mode, TCMalloc assigns each thread a thread-local cache. Small allocations are satisfied from this cache with no locking at all. A thread cache cntains one singly-linked list of free object per size class: if there are N size classes, there are N corresponding linked lists.
Objects migrate between thread caches and the central allocator as needed

The modern default is per-CPU mode: rather than per thread, each logical CPU has its own slab, and a restartable sequence mechanism(RESQ) allows the CPU-local slab to be updated without any locking in the common case
