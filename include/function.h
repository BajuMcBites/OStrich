#ifndef _FUNCTION_H
#define _FUNCTION_H

template <typename Signature>
class Function;  // Primary template (left undefined)

// Specialization for function signatures
template <typename R, typename... Args>
class Function<R(Args...)> {
   private:
    // Base class for type erasure
    struct CallableBase {
        virtual R call(Args... args) = 0;
        virtual ~CallableBase() {
        }
    };

    // Derived template class that wraps a callable (lambda, function pointer, functor)
    template <typename T>
    struct Callable : CallableBase {
        T func;
        Callable(const T& f) : func(f) {
        }
        R call(Args... args) override {
            return func(args...);
        }
    };

    CallableBase* callable;  // Pointer to callable base class

   public:
    // Constructor accepting any callable
    template <typename T>
    Function(T f) : callable(new Callable<T>(f)) {
    }

    // Move constructor
    Function(Function&& other) : callable(other.callable) {
        other.callable = nullptr;
    }

    // Move assignment
    Function& operator=(Function&& other) {
        if (this != &other) {
            callable = other.callable;
            other.callable = nullptr;
        }
        return *this;
    }

    // Deleting copy constructor & copy assignment to avoid deep copying
    Function(const Function& other) {
        if (this != &other) {
            callable = other.callable;
        }
    }
    Function& operator=(const Function&) = delete;

    // Call operator
    R operator()(Args... args) {
        if (callable) return callable->call(args...);
        return;
    }

    // Destructor
    ~Function() {
    }
};
#endif