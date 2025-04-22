#include "string.h"

#include "filesys_compat/vector"
#include "libk.h"
#include "printf.h"

string::string() {
    this->container.push_back('\0');
}

string::string(const string& other) {
    // Do a deep copy
    *this += other;
}

string::string(string&& other) {
    this->container = K::move(other.container);
}

string::string(const char* c_str) {
    if (c_str == nullptr) return;

    int i = 0;
    while (c_str[i] != '\0') {
        container.push_back(c_str[i++]);
    }
    container.push_back('\0');
}

string& string::operator=(const string& other) {
    // Guard against self assignment.
    if (this == &other) return *this;

    // Deep copy.
    this->container = other.container;

    return *this;
}

string& string::operator=(string&& other) {
    if (this != &other) {
        this->container = K::move(other.container);
    }
    return *this;
}

const char& string::operator[](size_t idx) const {
    return this->container[idx];
}

char& string::operator[](size_t idx) {
    return this->container[idx];
}

string& string::operator+=(const char& character) {
    this->container.back() = character;
    this->container.push_back('\0');
    return *this;
}

string& string::operator+=(const string& other) {
    for (size_t i = 0; i < other.length(); i++) {
        *this += other[i];
    }
    return *this;
}

uint64_t string::hash() const {
    constexpr uint64_t prime = 31;
    constexpr uint64_t mod = 1000000007;
    uint64_t hash = 0;
    uint64_t prime_pow = 1;
    for (int i = 0; i < this->length(); i++) {
        hash = (hash + prime_pow * (static_cast<uint8_t>(this->container[i]) + 1)) % mod;
        prime_pow = (prime_pow * prime) % mod;
    }
    return hash;
}

string operator+(const string& lhs, const string& rhs) {
    string result(lhs);
    result += rhs;
    return result;
}

string operator+(string&& lhs, const string& rhs) {
    lhs += rhs;
    return K::move(lhs);
}

size_t string::length() const {
    return this->container.size() - 1;
}

bool string::operator==(const string& other) const {
    if (this->length() != other.length()) {
        return false;
    }

    for (size_t i = 0; i < this->length(); i++) {
        if (this->container[i] != other.container[i]) {
            return false;
        }
    }
    return true;
}

bool string::starts_with(const string& other) const {
    if (this->length() < other.length()) {
        return false;
    }

    for (size_t i = 0; i < other.length(); i++) {
        if (this->container[i] != other.container[i]) {
            return false;
        }
    }
    return true;
}
void stringTest() {
    // Construction tests
    string a("Hello");
    string b("World");
    string empty("");
    string null(nullptr);

    // Test length
    K::assert(a.length() == 5, "string a should have length 5");
    K::assert(empty.length() == 0, "empty string should have length 0");
    K::assert(null.length() == 0, "null string should have length 0");

    printf("length test done\n");

    K::assert(a[0] == 'H', "a[0] should be 'H'");
    K::assert(a[4] == 'o', "a[4] should be 'o'");

    printf("indexing test done\n");

    // Test copy assignment
    string c = a;
    K::assert(c == a, "copied string c should equal original a");
    K::assert(c[0] == 'H', "first char of c should be 'H'");

    printf("copty assignment done\n");

    // Test self assignment
    c = c;
    K::assert(c == a, "self-assigned string c should still equal a");

    printf("self assignment done\n");

    // Test move assignment
    string d("Temporary");
    string e = K::move(d);
    printf("created temporary string\n");
    K::assert(e[0] == 'T', "moved string e should have 'T' at index 0");

    printf("move assignment done\n");

    // Test append character
    string f("Test");
    f += '!';
    K::assert(f.length() == 5, "string f should have length 5 after appending '!'");
    K::assert(f[4] == '!', "last char of f should be '!'");
    printf("append character done\n");

    // Test append string
    string g("Hello");
    g += string(" World");
    K::assert(g.length() == 11, "string g should have length 11 after appending ' World'");
    K::assert(g == string("Hello World"), "g should equal 'Hello World'");
    printf("append string done\n");

    // Test concatenation
    string h = a + b;
    K::assert(h == string("HelloWorld"), "concatenated string h should be 'HelloWorld'");
    printf("concatenation done\n");

    string i = string("Move") + string("Test");
    K::assert(i == string("MoveTest"), "concatenated string i should be 'MoveTest'");
    printf("concatenation done\n");

    // Test equality
    string j("Hello");
    K::assert(a == j, "a should equal j");
    K::assert(!(a == b), "a should not equal b");
    printf("equality test done\n");

    // Test hash function
    string k("Hello");
    string l("Hello");
    string m("World");
    K::assert(k.hash() == l.hash(), "identical strings should have equal hashes");
    K::assert(k.hash() != m.hash(), "different strings should have different hashes");
    printf("hash test done\n");

    // Print test results
    // You might want to implement a toString() method or overload << for proper output
    printf("String tests passed!\n");
}

const char* string::c_str() const {
    return this->container.data_ptr();
}