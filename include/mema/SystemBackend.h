#ifndef SYSTEM_BACKEND_H
#define SYSTEM_BACKEND_H

#include <cstddef>
#include <new>

struct SystemBackend {
    
    static void* Allocate(std::size_t n) {
        void* p = std::malloc(n);
        if (!p && n != 0) throw std::bad_alloc();
        return p;
    }

    static void Free(void* p, std::size_t) noexcept {
        std::free(p);
    }
};


#endif // !SYSTEM_BACKEND_H
