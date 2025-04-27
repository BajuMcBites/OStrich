#ifndef _shared_h_
#define _shared_h_

template <typename T>
class Shared {
    T* ptr;

    void drop() {
        if (ptr != nullptr) {
            auto new_count = ptr->ref_count.add_fetch(-1);
            if (new_count == 0) {
                delete ptr;
                ptr = nullptr;
            }
        }
    }

    void add() {
        if (ptr != nullptr) {
            ptr->ref_count.add_fetch(1);
        }
    }

   public:
    explicit Shared(T* it) : ptr(it) {
        add();
    }

    //
    // Shared<Thing> a{};
    //
    Shared() : ptr(nullptr) {
    }

    //
    // Shared<Thing> b { a };
    // Shared<Thing> c = b;
    // f(b);
    // return c;
    //
    Shared(const Shared& rhs) : ptr(rhs.ptr) {
        add();
    }

    //
    // Shared<Thing> d = g();
    //
    Shared(Shared&& rhs) : ptr(rhs.ptr) {
        rhs.ptr = nullptr;
    }

    ~Shared() {
        drop();
    }

    // d->m();
    T* operator->() const {
        return ptr;
    }

    // d = nullptr;
    // d = new Thing{};
    Shared<T>& operator=(T* rhs) {
        if (this->ptr != rhs) {
            drop();
            this->ptr = rhs;
            add();
        }
        return *this;
    }

    // d = a;
    // d = Thing{};
    Shared<T>& operator=(const Shared<T>& rhs) {
        auto other_ptr = rhs.ptr;
        if (ptr != other_ptr) {
            drop();
            ptr = other_ptr;
            add();
        }
        return *this;
    }

    // d = g();
    Shared<T>& operator=(Shared<T>&& rhs) {
        drop();
        ptr = rhs.ptr;
        rhs.ptr = nullptr;

        return *this;
    }

    bool operator==(const Shared<T>& rhs) const {
        return ptr == rhs.ptr;
    }

    bool operator!=(const Shared<T>& rhs) const {
        return ptr != rhs.ptr;
    }

    bool operator==(T* rhs) {
        return ptr == rhs;
    }

    bool operator!=(T* rhs) {
        return ptr != rhs;
    }

    // e = Shared<Thing>::make(1,2,3);
    template <typename... Args>
    static Shared<T> make(Args... args) {
        return Shared<T>{new T(args...)};
    }
};

#endif