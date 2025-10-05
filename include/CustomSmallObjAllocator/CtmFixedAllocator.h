#ifndef CTM_FIXED_ALLOCATOR_H
#define CTM_FIXED_ALLOCATOR_H

#include <deque>
#include <vector>
#include <map>
#include "SmallObjAllocator\Chunk.h"

namespace soa {

	/// CtmFixedAllocator is a slightly modification of 
	/// the FixedAllocator by Alexandrescu with the aim of improving deallocation
	/// for Butterfly trends scenarios.
	/// 
	/// It uses std::deque to keep tracks of allocated chunks.
	/// - Insertion and deletion at either end of a deque never 
	///   invalidates pointers or references to the rest of the elements.
	/// - As opposed to std::vector, the elements of a deque are 
	///   not stored contiguously. Loss in locality.
	/// - Expansion of a deque is cheaper than the expansion of 
	///   a std::vector because it does not involve copying of the 
	///   existing elements to a new memory location. On the other hand,
	///   deques typically have large minimal memory cost.
	/// (For more see:https://en.cppreference.com/w/cpp/container/deque.html).
	/// 
	/// The reason for std::deque is to use std::map to find faster the
	/// right chunk when Deallocate is called and avoid invalidation of 
	/// pointers.
	/// 
	/// There is also a vector to "cache" some empty chunks and try to speed
	/// up newer allocations after a chunks is completely freed.
	/// 
	/// Additionaly there is m_numFullBlocks in order to avoid 

	class CtmFixedAllocator {

	private:

		std::size_t m_blockSize{};
		unsigned char m_numBlocks{};
		std::size_t m_numFullChunks{};

		std::deque<Chunk> m_chunks{};
		std::map<std::uintptr_t, Chunk*> m_chunkMap{};
		std::vector<Chunk*> m_freeChunks;

		Chunk* m_allocChunk = nullptr;
		Chunk* m_deallocChunk = nullptr;

		void DoDeallocate(void* p);

	public:

		explicit CtmFixedAllocator(std::size_t blockSize = 0);
		~CtmFixedAllocator();

		// avoid copies
		CtmFixedAllocator(const CtmFixedAllocator&) = delete;
		CtmFixedAllocator& operator=(const CtmFixedAllocator&) = delete;

		// move operations
		CtmFixedAllocator(CtmFixedAllocator&&)  noexcept;
		CtmFixedAllocator& operator=(CtmFixedAllocator&&)  noexcept;

		//void Swap(CtmFixedAllocator& rhs);

		void* Allocate();
		void  Deallocate(void* p);
		inline std::size_t GetBlockSize() const { return m_blockSize; }
	};

}

#endif //! CTM_FIXED_ALLOCATOR_H