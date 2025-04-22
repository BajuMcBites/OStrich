#ifndef UTILITY_H
#define UTILITY_H

namespace std {
// Allows you to call forward without a type specifier.
template <typename T>
T&& forward(T&& arg) {
    return static_cast<T&&>(arg);
}
}  // namespace std

#endif
