// -*- mode:c++; c-basic-offset:4 -*-
#include "util.h"

#include <sys/stat.h>
#include <sys/mman.h>
#include <cerrno>
#include <cassert>
#include <vector>
#include <utility>
#include <algorithm>
#include <aio.h>

#ifdef PAGE_SIZE
//#warning Removing PAGE_SIZE macro
#undef PAGE_SIZE
#endif

static const size_t PAGE_SIZE = 4096;

class prefetcher {
    
    
};

struct stream_key {
    int fd;
    size_t block;
    stream_key()
        : fd(0), block(0)
    {
    }
    stream_key(int fd_, size_t b)
        : fd(fd_), block(b)
    {
    }
    bool operator ==(stream_key const &other) const {
        return fd == other.fd && block == other.block;
    }
    bool operator !=(stream_key const &other) const {
        return !(*this == other);
    }
};

typedef std::pair<stream_key, char*> cache_entry;
typedef std::vector<cache_entry> disk_cache;
static char* buf;
static pthread_mutex_t cache_lock = thread_mutex_create();
static size_t cache_size=0;

static size_t stream_size; // in pages

static disk_cache cache;

static volatile bool aio_active = false;
static aiocb aio_request;
static aiocb* aio_list[] = {&aio_request};
static stream_key aio_marker;
static size_t aio_block;
 int prefetch_success_count;
 int prefetch_fail_count;

static const int LOCKED = -1;

extern "C"
int disk_cache_invalidate(int fd, size_t page, size_t size) {
    assert(size == PAGE_SIZE);
    return 0;
}

static bool check_aio_ready() {
    // pending?
    if(!aio_active)
        return true;

    // complete?
    int result = aio_error(&aio_request);
    if(result == EINPROGRESS)
        return false;

    // it's done, one way or another...
    aio_active = false;
    if(result != 0) {
        errno = result;
        perror("aio_error");
        return false;
    }

    // update the correct cache slot
    int i;
    for(i=cache.size(); i-- > 0 && cache[i].first != aio_marker; );
    assert(i >= 0);

    cache_entry &entry = cache[i];
    entry.first = stream_key(aio_request.aio_fildes, aio_block);
    return true;
}

extern "C"
int disk_cache_read(void* dest, int fd, size_t page, size_t size) {
        
    assert(size == PAGE_SIZE);
    int block = page & -stream_size;
    int block_offset = page - block;
    int byte_offset = block_offset*PAGE_SIZE;
    
    critical_section_t cs(cache_lock);
    
    // which block does this page belong to?
    stream_key request(fd, block);

    // hit?
 lookup:
    int i;
    for(i=0; i < (int) cache.size() && cache[i].first != request; i++);
    if(i < (int) cache.size()) {

        // bubble the entry to the front to preserve LRU ordering
        if(block_offset < (int) stream_size-1) {
            while(i > 0)
                std::swap(cache[i--], cache[i]);
        }
        else if(1) {
            // copy now, before giving away the memory
            cache_entry &old_entry = cache[i];
            memcpy(dest, &old_entry.second[byte_offset], PAGE_SIZE);

            // can we prefetch?
            bool aio_was_active = aio_active;
            if(check_aio_ready()) {
                if(aio_was_active)
                    prefetch_fail_count++;
                aio_marker = stream_key(LOCKED, i);
                aio_block = block+stream_size;

                memset(&aio_request, 0, sizeof(aio_request));
                aio_request.aio_fildes = fd;
                aio_request.aio_lio_opcode = LIO_READ;
                aio_request.aio_buf = old_entry.second;
                aio_request.aio_nbytes = stream_size*PAGE_SIZE;
                aio_request.aio_sigevent.sigev_notify = SIGEV_NONE;
                aio_request.aio_offset = aio_block*PAGE_SIZE;
                
                if(aio_read(&aio_request)) {
                    // problem?
                    perror("aio_read");
                    old_entry.first = stream_key();
                }
                else {
                    // successfully queued up
                    aio_active = true;
                    old_entry.first = aio_marker;
                }
            }
            else {
                // clear it out for someone else to use
                old_entry.first = stream_key();
                while(++i < (int) cache.size())
                    std::swap(cache[i], cache[i-1]);
            }
            
            return 0;
        }
    }

    // what about pending prefetch?
    else if(aio_active && aio_request.aio_fildes == fd && aio_block == block) {
        // try to wait for it
        cs.exit();
        int result = aio_suspend(aio_list, 1, NULL);
        int myerrno = errno;
        cs.enter(cache_lock);
        if(result) {
            errno = myerrno;
            perror("aio_suspend");
            goto fetch;
        }
        else if(check_aio_ready()) {
            prefetch_success_count++;
            goto lookup;
        }
        else
            goto fetch;
    }
    
    // miss.
    else {
    fetch:
        // find the oldest unlocked entry to use as a victim
        i = cache.size();
        while(i-- > 0 && cache[i].first.fd == LOCKED);
        if(i < 0) {
            // too many prefetches in progress. Drop this one on the floor.
            return -1;
        }

        // invalidate and mark it, then use its buffer
        stream_key marker(LOCKED, i);
        cache_entry &old_entry = cache[i];
        old_entry.first = marker;
        char* buf = old_entry.second;
    
        // release the lock during the I/O
        int target_count = stream_size*PAGE_SIZE;
        cs.exit();
        int count = pread(fd, buf, target_count, block*PAGE_SIZE);
        int myerrno = errno;

        // re-acquire the lock, then find our entry again (LRU could have
        // moved it while we were gone)
        cs.enter(cache_lock);
        for(i=cache.size(); i-- > 0 && cache[i].first != marker; );
        assert(i >= 0);

        cache_entry &entry = cache[i];
        // sanity check
        if(count != target_count) {
            if(count < 0) {
                errno = myerrno;
                perror("disk_cache_read");
                struct stat stat;
                fstat(fd, &stat);
            }
            
            // TODO: mask out invalid pages instead of failing outright
            // set the entry to a bogus value so it gets replaced
            entry.first = stream_key();
            // TODO: bubble it to the end so it gets replaced first
            return -2;
        }

        // success! update the stream key
        entry.first = request;

        // move this stream to the middle of the LRU stack so it
        // doesn't get replaced too quickly
        while(i > (int) cache.size()/2)
            std::swap(cache[i--], cache[i]);
    }

    // copy the data across and we're done
    cache_entry &entry = cache[i];
    memcpy(dest, &entry.second[byte_offset], PAGE_SIZE);
    return 0;
}
        
extern "C"
int disk_cache_init(size_t size, int streams) {
    cache_size = size;
    // must be at least 1 stream
    assert(streams >= 1);
    
    stream_size = cache_size/streams/PAGE_SIZE;
    // each stream must contain an integral number of pages...
    assert(stream_size*streams*PAGE_SIZE == cache_size);
    // ...that is greater than 1 ...
    assert(stream_size > 1);
    // ...and a power of two...
    assert((stream_size & -stream_size) == stream_size);

    // allocate the memory
    buf = (char*) mmap(0, size,
                       PROT_READ|PROT_WRITE,
                       MAP_PRIVATE|MAP_ANONYMOUS|MAP_POPULATE,
                       0, 0);
    if(buf == MAP_FAILED) {
        perror("init_disk_cache");
        return errno;
    }

    // populate the cache with unaligned
    stream_key dummy;
    for(int i=0; i < streams; i++)
        cache.push_back(std::make_pair(dummy, &buf[i*stream_size*PAGE_SIZE]));
    
    return 0;
}

extern "C"
int disk_cache_destroy() {
    if(cache_size != 0) {
        if(munmap(buf, cache_size)) {
            perror("destroy_disk_cache");
            return errno;
        }
    }

    return 0;
}

