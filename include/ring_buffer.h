#ifndef _HASH_H
#define _HASH_H

#include "heap.h"
#include "atomic.h"

template <typename T>
class RingBuffer {
    public:
    size_t size;
    Lock lock;
    Semaphore* read_sema;
    Semaphore* write_sema;


    RingBuffer(size_t size) {
        this.size = size;
        container = kmalloc(size * sizeof(T));
        // sema = new Semaphore(0);
        read_sema = new Semaphore(0);
        write_sema = new Semaphore(size);
    }

    ~RingBuffer() {
        delete read_sema;
        delete write_sema;
        kfree(container);
    }

    void read(Funct) {
            read_sema->down([=]() {
                lock.lock([=]() {
                    
                    

                    lock.unlock();
                });
            });
    }

    
    void write(T item) {
        write_sema->down([=]()) {
        lock.lock([=] ()) {
            if (write_pointer - size == read_pointer)
        }
    }
    
    private:
    T *container;
    uint64_t read_pointer = 0;
    uint64_t write_pointer = 0;

}

#endif /*_HASH_H*/