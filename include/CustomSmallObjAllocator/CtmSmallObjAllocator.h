#ifndef CUSTOM_SMALL_OBJ_ALLOC_H
#define CUSTOM_SMALL_OBJ_ALLOC_H

#include <vector>
#include "SmallObjAllocator\SOA_defaults.h"
#include "CtmFixedAllocator.h"

namespace soa {

	class CtmSmallObjAllocator {
	public:
		CtmSmallObjAllocator(std::size_t chunkSize, std::size_t maxObjectSize);

		static CtmSmallObjAllocator& Instance() noexcept
		{
			static CtmSmallObjAllocator smallObjAllocator(DEFAULT_CHUNK_SIZE, DEFAULT_MAX_OBJ_SIZE);
			return smallObjAllocator;
		};

		void* Allocate(std::size_t numBytes);
		void  Deallocate(void* p, std::size_t size);

	private:
		CtmSmallObjAllocator(const CtmSmallObjAllocator& i_other) = delete;
		CtmSmallObjAllocator& operator=(const CtmSmallObjAllocator& i_other) = delete;
		CtmSmallObjAllocator(const CtmSmallObjAllocator&& i_other) = delete;
		CtmSmallObjAllocator& operator=(const CtmSmallObjAllocator&& i_other) = delete;

		std::vector<CtmFixedAllocator> m_Pool{};
		CtmFixedAllocator* m_pLastAlloc{};
		CtmFixedAllocator* m_pLastDealloc{};

		std::size_t m_chunkSize{};
		std::size_t m_maxObjSize{};
	};
}

#endif // !CUSTOM_SMALL_OBJ_ALLOC_H