#ifndef _TUPLE_H
#define _TUPLE_H

#include <stddef.h>

namespace tuple {
// Recursive structure to find the index of a type in the parameter pack
template <typename T, typename... Types>
struct index_of_impl;

// Base case: when the type is found, return 0
template <typename T, typename... Rest>
struct index_of_impl<T, T, Rest...> {
    static constexpr size_t value = 0;
};

// Recursive case: check the next type in the pack
template <typename T, typename First, typename... Rest>
struct index_of_impl<T, First, Rest...> {
    static constexpr size_t value = 1 + index_of_impl<T, Rest...>::value;
};

// If the type is not found, this will be used as a fallback
template <typename T>
struct index_of_impl<T> {
    static constexpr size_t value = -1;
};

// Public interface to get the index of a type in a parameter pack
template <typename T, typename... Types>
constexpr size_t index_of() {
    return index_of_impl<T, Types...>::value;
}
};  // namespace tuple

#endif