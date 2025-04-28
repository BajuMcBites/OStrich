#ifndef _STRING_H
#define _STRING_H

#include "filesys_compat/vector"
#include "stdint.h"

class string {
   public:
    string(const char* c_str);
    string(const string& other);
    string(string&& other);
    string();
    ~string() = default;

    // Assignment operators
    string& operator=(const string& other);  // copy assignment
    string& operator=(string&& other);       // move assignment

    // Indexing
    const char& operator[](size_t idx) const;
    char& operator[](size_t idx);

    // Comparison
    bool operator==(const string& other) const;

    // Concatentation
    string& operator+=(const char& character);
    string& operator+=(const string& other);

    // Steal buffers from temporary values via move semantics.
    friend string operator+(const string& lhs, const string& rhs);
    friend string operator+(string&& lhs, const string& rhs);

    // Capacity
    bool is_empty() const;
    size_t length() const;
    size_t max_size() const;

    // Hashing
    uint64_t hash() const;

    // Nice to have
    bool starts_with(const string& other) const;
    const char* c_str() const;

   private:
    std::vector<char> container;
};

void stringTest();
#endif
