#ifndef _RING_BUFFER_H
#define _RING_BUFFER_H

#include "heap.h"
#include "atomic.h"
#include "event.h"

template <typename T>
class RingBuffer {
    public:
    size_t size;

    RingBuffer(size_t size) {
        this->size = size;
        container = (T*) kmalloc(size * sizeof(T));
        read_sema = new Semaphore(0);
        write_sema = new Semaphore(size);
        lock = new Lock;
    }

    ~RingBuffer() {
        delete read_sema;
        delete write_sema;
        kfree(container);
    }

    void read(Function<void (T result)> func) {
        read_sema->down([=]() {
            lock->lock([=]() {
                T item = container[read_pointer];
                read_pointer = (read_pointer + 1) % size;

                lock->unlock();
                write_sema->up();
                create_event<T>(func, item);
            });
        });
    }
    
    void write(T item, Function<void ()> func) {
        write_sema->down([=]() {
            lock->lock([=] () {
                container[write_pointer] = item;
                write_pointer = (write_pointer + 1) % size;

                lock->unlock();
                read_sema->up();

                create_event(func);
            });
        });
    }
    
    private:
    T *container;
    Semaphore *read_sema;
    Semaphore *write_sema;
    Lock* lock;
    uint64_t read_pointer = 0;
    uint64_t write_pointer = 0;

};

#endif /*_RING_BUFFER_H*/