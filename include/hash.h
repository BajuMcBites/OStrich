#include "heap.h"

template <typename T>
struct HashNode {
    uint64_t key;
    T value;
    struct HashNode *next;
};

template <typename T>
class Hashmap {
   public:
    unsigned long size;

    Hashmap(unsigned long size = 100) {
        if (size < 1) size = 1;

        this->size = 0;
        true_size = size;
        container = (HashNode<T> **)kcalloc(0, sizeof(struct HashNode<T>) * true_size);
    }

    ~Hashmap() {
        for (unsigned long i = 0; i < true_size; i++) {
            struct HashNode<T> *cur = container[i];
            struct HashNode<T> *temp;

            while (cur) {
                temp = cur->next;
                kfree(cur);
                cur = temp;
            }
        }

        kfree(container);
    }

    T get(uint64_t key) {
        unsigned long index = key % true_size;

        if (container[index] != nullptr) {
            struct HashNode<T> *cur = container[index];
            while (cur->next) {
                if (cur->key == key) return cur->value;

                cur = cur->next;
            }

            return cur->key == key ? cur->value : nullptr;
        }

        return nullptr;
    }

    bool put(uint64_t key, T value) {
        if (size * 1.0 / true_size >= .75) resize();

        if (insert(key, value)) size++;

        return true;
    }

    bool remove(uint64_t key) {
        unsigned long index = key % true_size;

        if (container[index] == nullptr) return false;

        struct HashNode<T> *cur = container[index];

        if (cur->key == key) {
            container[index] = cur->next;
            kfree(cur);
            size -= 1;
            return true;
        }

        while (cur->next && cur->next->key != key) cur = cur->next;

        if (cur->next && cur->next->key == key) {
            cur->next = cur->next->next;
            kfree(cur);
            size -= 1;
            return true;
        }

        return false;
    }

   private:
    unsigned long true_size;
    struct HashNode<T> **container;

    struct HashNode<T> *create_node(uint64_t key, T value) {
        struct HashNode<T> *node = (struct HashNode<T> *)kmalloc(sizeof(struct HashNode<T> *));
        node->key = key;
        node->value = value;
        node->next = nullptr;
        return node;
    }

    bool insert(uint64_t key, T value) {
        unsigned long index = key % true_size;

        if (container[index] == nullptr) {
            container[index] = create_node(key, value);
            return true;
        }

        struct HashNode<T> *cur = container[index];
        while (cur->next && cur->key != key) cur = cur->next;

        if (cur->key == key) {
            cur->value = value;
            return false;
        }

        cur->next = create_node(key, value);

        return true;
    }

    void reinsert(struct HashNode<T> *node) {
        unsigned long index = node->key % true_size;

        node->next = nullptr;

        if (container[index] == nullptr) {
            container[index] = node;
            return;
        }

        struct HashNode<T> *cur = container[index];
        while (cur->next) cur = cur->next;

        cur->next = node;

        return;
    }

    void resize() {
        unsigned long prev_size = true_size;
        true_size *= 2;
        struct HashNode<T> *cur;
        struct HashNode<T> *temp;
        struct HashNode<T> **prev = container;
        container = (HashNode<T> **)kcalloc(0, sizeof(struct HashNode<T>) * true_size);

        for (unsigned long i = 0; i < prev_size; i++) {
            cur = prev[i];
            while (cur) {
                temp = cur->next;
                reinsert(cur);
                cur = temp;
            }
        }

        kfree(prev);
    }
};

#endif /*_HASH_H*/