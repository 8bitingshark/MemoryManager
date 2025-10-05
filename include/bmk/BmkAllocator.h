#ifndef BMK_ALLOCATOR_H
#define BMK_ALLOCATOR_H

#include <cstddef>  // std::size_t
#include <utility>  // std::forward

namespace bmk {

    template<typename Backend>
    class BmkAllocator {
    public:
        void* Allocate(std::size_t size) { return Backend::Allocate(size); }
        void Free(void* p, std::size_t size) { Backend::Free(p, size); }

        template<typename T, typename... Args>
        T* New(Args&&... args) {
            void* mem = Allocate(sizeof(T));
            return new (mem) T(std::forward<Args>(args)...);
        }

        template<typename T>
        void Delete(T* obj) {
            if (!obj) return;
            obj->~T();
            Free(obj, sizeof(T));
        }
    };
}

#endif // !BMK_ALLOCATOR_H