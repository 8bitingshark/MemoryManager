#ifndef SMALL_OBJ_ALLOCATOR_H
#define SMALL_OBJ_ALLOCATOR_H

#include <vector>
#include "SOA_defaults.h"
#include "FixedAllocator.h"

namespace soa {

	/// SmallObjAllocator from Modern C++ Desing by Alexandrescu
	///
	/// - Holds several FixedAllocator objects, 
	///   each specialized for allocating objects of one size.
	/// - When SmallObjAllocator receives an allocation request, 
	///   it either forwards it to the best matching FixedAllocator 
	///   object or passes it to the default ::operator new.
	/// 
	/// Oddly enough, Deallocate takes the size to deallocate as an argument. 
	/// Deallocations are much faster this way.
	/// It gives to the user the task to remember the size for the allocation
	/// to increase performance.
	/// 
	/// How Pool can manage fixed allocators?
	/// A simple and efficient idea is to have m_Pool[i] to handle objects 
	/// of size i. Allocation and deallocation constant-time operation.
	/// 
	/// Efficient is not always effective!
	/// It depends from the size of your objects, if you have only 2 and 64
	/// bytes, you need to allocate all the other fixed allocators.
	/// 
	/// Alignment and padding issues further contribute to wasting space m_Pool{}. 
	/// For instance, many compilers pad all user defined types up to a multiple of a 
	/// number(2, 4, or more).
	/// If the compiler pads all structures to a multiple of 4, for instance, 
	/// you can count on using only 25 % of pool_—the rest is wasted.
	/// 
	/// A better approach is to sacrifice a bit of lookup speed for 
	/// the sake of memory conservation.
	/// - Keep only sizes that are one requested
	/// - Pool can accomodate objects of varyous sizes without growing so much
	/// - Improvement: keep sorted by size
	/// 
	/// Use the same strategy of FixedAllocators to improve lookup speed

	class SmallObjAllocator {
	public:
		SmallObjAllocator(std::size_t chunkSize, std::size_t maxObjectSize);

		static SmallObjAllocator& Instance() noexcept
		{
			static SmallObjAllocator smallObjAllocator(DEFAULT_CHUNK_SIZE, DEFAULT_MAX_OBJ_SIZE);
			return smallObjAllocator;
		};

		void* Allocate(std::size_t numBytes);
		void  Deallocate(void* p, std::size_t size);

	private:
		SmallObjAllocator(const SmallObjAllocator& i_other) = delete;
		SmallObjAllocator& operator=(const SmallObjAllocator& i_other) = delete;
		SmallObjAllocator(const SmallObjAllocator&& i_other) = delete;
		SmallObjAllocator& operator=(const SmallObjAllocator&& i_other) = delete;

		std::vector<FixedAllocator> m_Pool{};
		FixedAllocator* m_pLastAlloc{};
		FixedAllocator* m_pLastDealloc{};

		std::size_t m_chunkSize{};
		std::size_t m_maxObjSize{};
	};
}

#endif // !SMALL_OBJ_ALLOCATOR_H
