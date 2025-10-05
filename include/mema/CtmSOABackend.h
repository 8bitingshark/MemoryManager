#ifndef CUSTOM_SOA_BACKEND_H
#define CUSTOM_SOA_BACKEND_H

#include "CustomSmallObjAllocator\CtmSmallObjAllocator.h"

namespace mema {

    struct CtmSOABackend {

        static void* Allocate(std::size_t size) noexcept {
            if (size == 0) return nullptr;
            return soa::CtmSmallObjAllocator::Instance().Allocate(size);
        }

        static void Free(void* p, std::size_t size) noexcept {

            if (!p) return;

            soa::CtmSmallObjAllocator::Instance().Deallocate(p, size);
            return;
        }
    };
}


#endif // !SOA_BACKEND_H