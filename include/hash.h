#ifndef _HASH_H
#define _HASH_H

#define FNV_OFFSET_BASIS 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

#define HASH_COMBINE_BASIS 0x9e3779b97f4a7c15ULL

#include "heap.h"
#include "libk.h"

static uint64_t uint64_t_hash(uint64_t elem) {
    elem = ((elem >> 16) ^ elem) * 0x45d9f3b;
    elem = ((elem >> 16) ^ elem) * 0x45d9f3b;
    elem = (elem >> 16) ^ elem;
    return elem;
}

static bool uint64_t_equals(uint64_t elem1, uint64_t elem2) {
    return elem1 == elem2;
}

/*
static uint64_t string_hash(char* str) {
    unsigned long long hash = FNV_OFFSET_BASIS;
    while (*str) {
        hash ^= (unsigned char)*str++;
        hash *= FNV_PRIME;
    }
    return (uint64_t) hash;
}

static bool string_equals(char* stra, char* strb) {
    return K::strncmp(stra, strb, PAGE_SIZE) == 0;
}
*/

inline uint64_t hash_combine(uint64_t h1, uint64_t h2) {
    return h1 ^ (h2 + HASH_COMBINE_BASIS + (h1 << 6) + (h1 >> 2));
}

template <typename K, typename T>
struct HashNode {
    K key;
    T value;
    struct HashNode *next;
};

template <typename K, typename T>
class HashMap {
   public:
    unsigned long size;

    HashMap(uint64_t (*hash_func)(K), bool (*equals_func)(K, K), unsigned long size = 100) {
        if (size < 1) size = 1;

        this->size = 0;
        this->hash_func = hash_func;
        this->equals_func = equals_func;
        true_size = size;
        container = (HashNode<K, T> **)kcalloc(sizeof(struct HashNode<K, T>), true_size);
    }

    HashMap(HashMap<K, T> *hashmap) {
        clone(hashmap);
    }

    ~HashMap() {
        for (unsigned long i = 0; i < true_size; i++) {
            struct HashNode<K, T> *cur = container[i];
            while (cur) {
                struct HashNode<K, T> *temp = cur;
                cur = cur->next;
                kfree(temp);
            }
        }

        kfree(container);
    }

    T get(K key) {
        unsigned long index = hash_func(key) % true_size;

        if (container[index] != nullptr) {
            struct HashNode<K, T> *cur = container[index];
            while (cur->next) {
                if (equals_func(cur->key, key)) return cur->value;

                cur = cur->next;
            }
            return equals_func(cur->key, key) ? cur->value : nullptr;
        }

        return not_found;
    }

    bool put(K key, T value) {
        if (size * 1.0 / true_size >= .75) resize();

        if (insert(key, value)) size++;

        return true;
    }

    bool remove(K key) {
        unsigned long index = hash_func(key) % true_size;

        if (container[index] == nullptr) return false;

        struct HashNode<K, T> *cur = container[index];

        if (equals_func(cur->key, key)) {
            container[index] = cur->next;
            kfree(cur);
            size -= 1;
            return true;
        }

        while (cur->next && !equals_func(cur->next->key, key)) cur = cur->next;

        if (cur->next && equals_func(cur->next->key, key)) {
            cur->next = cur->next->next;
            kfree(cur);
            size -= 1;
            return true;
        }

        return false;
    }

    void for_each(Function<void(T t)> consumer) {
        for (unsigned long i = 0; i < true_size; i++) {
            struct HashNode<K, T> *cur = container[i];
            struct HashNode<K, T> *temp;

            while (cur) {
                temp = cur->next;
                consumer(cur->value);
                cur = temp;
            }
        }
    }

    void set_not_found(T value) {
        not_found = value;
    }

   private:
    T not_found = nullptr;
    uint64_t (*hash_func)(K);
    bool (*equals_func)(K, K);
    unsigned long true_size;
    struct HashNode<K, T> **container;

    struct HashNode<K, T> *create_node(K key, T value) {
        struct HashNode<K, T> *node =
            (struct HashNode<K, T> *)kmalloc(sizeof(struct HashNode<K, T>));
        node->key = key;
        node->value = value;
        node->next = nullptr;
        return node;
    }

    struct HashNode<K, T> *duplicate_node(struct HashNode<K, T> *node) {
        struct HashNode<K, T> *copy =
            (struct HashNode<K, T> *)kmalloc(sizeof(struct HashNode<K, T>));
        copy->key = node->key;
        copy->value = node->value;
        copy->next = nullptr;
        return copy;
    }

    bool insert(K key, T value) {
        unsigned long index = hash_func(key) % true_size;

        if (container[index] == nullptr) {
            container[index] = create_node(key, value);
            return true;
        }

        struct HashNode<K, T> *cur = container[index];
        while (cur->next && !equals_func(cur->key, key)) cur = cur->next;

        if (equals_func(cur->key, key)) {
            cur->value = value;
            return false;
        }

        cur->next = create_node(key, value);
        return true;
    }

    void reinsert(struct HashNode<K, T> *node) {
        unsigned long index = hash_func(node->key) % true_size;

        node->next = nullptr;
        if (container[index] == nullptr) {
            container[index] = node;
            return;
        }

        struct HashNode<K, T> *cur = container[index];
        while (cur->next) cur = cur->next;

        cur->next = node;
        return;
    }

    void resize() {
        unsigned long prev_size = true_size;
        true_size *= 2;
        struct HashNode<K, T> **prev = container;
        container = (struct HashNode<K, T> **)kcalloc(sizeof(struct HashNode<K, T>), true_size);

        for (unsigned long i = 0; i < prev_size; i++) {
            struct HashNode<K, T> *cur = prev[i];
            while (cur) {
                struct HashNode<K, T> *temp = cur->next;
                reinsert(cur);
                cur = temp;
            }
        }

        kfree(prev);
    }

    void clone(HashMap<K, T> *hashmap) {
        this->size = hashmap->size;
        this->true_size = hashmap->true_size;
        this->hash_func = hashmap->hash_func;
        this->equals_func = hashmap->equals_func;
        container = (HashNode<K, T> **)kcalloc(sizeof(struct HashNode<K, T>), true_size);

        for (int i = 0; i < true_size; i++) {
            struct HashNode<K, T> *other_cur = hashmap->container[i];
            if (other_cur != nullptr) {
                struct HashNode<K, T> *cur = duplicate_node(other_cur);
                container[i] = cur;
                while (other_cur->next) {
                    cur->next = duplicate_node(other_cur->next);
                    cur = cur->next;
                    other_cur = other_cur->next;
                }
            }
        }
    }
};

#endif /*_HASH_H*/