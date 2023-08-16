#include <iostream>
#include <string>
#include <cstdint>
#include <limits>
#include <bit>

struct Large {
    unsigned char* ptr;
    uint64_t size;
};

constexpr uint64_t BOTTOM_63_MASK = std::numeric_limits<int64_t>::max();
constexpr unsigned char SMALL_CHARACTER_SIZE = 15;
constexpr unsigned char FIRST_ALLOC_SIZE = 32;

struct Small {
    unsigned char characters[SMALL_CHARACTER_SIZE];
    unsigned char size : 7;
    unsigned char flag : 1;
};

union _MicroString {
    Small S;
    Large L;
};

class MicroString {
    _MicroString MS;
    
    bool isSmall() const noexcept {
        return MS.S.flag == 0;
    }
    uint64_t getSize() const noexcept {
        if (isSmall()) {
            return SMALL_CHARACTER_SIZE - MS.S.size;
        }
        return MS.L.size & BOTTOM_63_MASK;
    }
    const unsigned char* getPtr() const noexcept {
        if (isSmall()) {
            return (unsigned char*)&MS;
        }
        return MS.L.ptr;
    }
    void initializeAsSmall() {
        MS.L.ptr = 0;
        MS.L.size = 0;
        MS.S.size = SMALL_CHARACTER_SIZE;
    }
public:
    MicroString() {
        initializeAsSmall();
    }

    MicroString(const char* str) {
        initializeAsSmall();
        std::size_t i = 0;
        while (str[i] != '\0') {
            push(str[i]);
            i += 1;
        }
    }

    MicroString(const char* str, std::size_t size) {
        initializeAsSmall();
        for (std::size_t i = 0; i < size; i++) {
            push(str[i]);
        }
    }

    MicroString(const std::string& str) {
        initializeAsSmall();
        for (const auto c : str) {
            push(c);
        }
    }

    MicroString(const MicroString& other) {
        if (other.isSmall()) {
            MS.S.flag = other.MS.S.flag;
            MS.S.size = other.MS.S.size;
            for (std::size_t i = 0; i < SMALL_CHARACTER_SIZE; i++) {
                MS.S.characters[i] = other.MS.S.characters[i];
            }
        }
        else {
            initializeAsSmall();
            for (std::size_t i = 0; i < other.size(); i++) {
                push(other.at(i));
            }
        }
    }

    ~MicroString() {
        if (!isSmall()) {
            delete[] MS.L.ptr;
        }
    }

    void push(unsigned char c) noexcept {
        const auto size = getSize();
        if (getSize() < SMALL_CHARACTER_SIZE) {
            // Add to small string
            MS.S.characters[size] = c;
            MS.S.size -= 1;
        } else if (getSize() == SMALL_CHARACTER_SIZE) {
            // Convert from small to large
            unsigned char* ptr = new unsigned char[FIRST_ALLOC_SIZE];
            for (std::size_t i = 0; i < SMALL_CHARACTER_SIZE; i++) {
                ptr[i] = MS.S.characters[i];
            }
            ptr[SMALL_CHARACTER_SIZE] = c;
            ptr[SMALL_CHARACTER_SIZE + 1] = '\0';
            MS.L.ptr = ptr;
            MS.L.size = SMALL_CHARACTER_SIZE + 1;
            MS.S.flag = 1;
        }
        else {
            // Add to large string
            const auto newSize = size + 2;
            if (newSize < FIRST_ALLOC_SIZE) {
                MS.L.ptr[size] = c;
                MS.L.ptr[size + 1] = '\0';
                MS.L.size += 1;
            }
            else {
                if ((newSize & (newSize - 1)) == 0) {
                    // Double allocated size if newSize is a power of 2
                    unsigned char* oldPtr = MS.L.ptr;
                    MS.L.ptr = new unsigned char[newSize * 2];
                    for (std::size_t i = 0; i < size; i++) {
                        MS.L.ptr[i] = oldPtr[i];
                    }
                    delete[] oldPtr;
                }

                MS.L.ptr[size] = c;
                MS.L.ptr[size + 1] = '\0';
                MS.L.size += 1;
            }
        }
    }

    unsigned char at(std::size_t i) const noexcept {
        if (isSmall()) {
            return MS.S.characters[i];
        }
        return MS.L.ptr[i];
    }

    unsigned char pop() noexcept {
        const auto size = getSize();
        if (size <= SMALL_CHARACTER_SIZE) {
            MS.S.size += 1;
            const unsigned char c = MS.S.characters[size - 1];
            MS.S.characters[size - 1] = '\0';
            return c;
        }
        else {
            const auto newSize = size - 1;
            unsigned char c = MS.L.ptr[newSize];
            if (newSize == SMALL_CHARACTER_SIZE) {
                // Convert to small if need be
                const unsigned char* ptr = MS.L.ptr;
                for (std::size_t i = 0; i < SMALL_CHARACTER_SIZE; i++) {
                    MS.S.characters[i] = ptr[i];
                }
                MS.S.size = 0;
                MS.S.flag = 0;
                return c;
            }

            if ((newSize & (newSize - 1)) == 0) {
                // Half allocated size if newSize is a power of 2
                unsigned char* oldPtr = MS.L.ptr;
                MS.L.ptr = new unsigned char[newSize + 1];
                for (std::size_t i = 0; i < size; i++) {
                    MS.L.ptr[i] = oldPtr[i];
                }
                delete[] oldPtr;
            }

            MS.L.ptr[newSize] = '\0';
            MS.L.size -= 1;
            return c;
        }
    }

    const std::size_t size() const noexcept {
        return getSize();
    }

    friend std::ostream& operator<<(std::ostream& os, const MicroString& ms) noexcept {
        os << ms.getPtr();
        return os;
    }
};


int main()
{
    std::cout << sizeof(MicroString) << "\n";
}
