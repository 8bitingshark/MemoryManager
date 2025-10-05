#ifndef SOA_BACKEND_H
#define SOA_BACKEND_H

#include "SmallObjAllocator\SmallObjAllocator.h"

namespace mema {

    struct SOABackend {
        
        static void* Allocate(std::size_t size) noexcept {
            if (size == 0) return nullptr;
            return soa::SmallObjAllocator::Instance().Allocate(size);
        }

        static void Free(void* p, std::size_t size) noexcept {
            
            if (!p) return;

            soa::SmallObjAllocator::Instance().Deallocate(p, size);
            return;
        }
    };
}


#endif // !SOA_BACKEND_H