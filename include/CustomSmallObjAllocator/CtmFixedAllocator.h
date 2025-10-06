#ifndef CTM_FIXED_ALLOCATOR_H
#define CTM_FIXED_ALLOCATOR_H

#include <deque>
#include <vector>
#include <map>
#include "SmallObjAllocator\Chunk.h"

namespace soa {

	/// CtmFixedAllocator is a slightly modified version of 
	/// Alexandrescu’s FixedAllocator, designed to improve the speed 
	/// of allocation and deallocation, especially in butterfly 
	/// (sub)trend scenarios.
	/// 
	/// It uses std::deque to keep tracks of allocated chunks.
	/// - Insertion and deletion at either end of a deque never 
	///   invalidates pointers or references to the rest of the elements.
	/// - As opposed to std::vector, the elements of a deque are 
	///   not stored contiguously.
	/// - Expansion of a deque is cheaper than the expansion of 
	///   a std::vector because it does not involve copying of the 
	///   existing elements to a new memory location. On the other hand,
	///   deques typically have large minimal memory cost.
	/// (For more see:https://en.cppreference.com/w/cpp/container/deque.html).
	/// 
	/// Using std::deque in this context allows the allocator to safely use 
	/// an std::map for faster lookup of the appropriate chunk during deallocation,
	/// while avoiding pointer invalidation.
	/// 
	/// A separate std::vector is also maintained to cache empty chunks,
	/// and trying to speed up newer allocations.
	/// 
	/// Additionally, the m_numFullBlocks member tracks how many chunks 
	/// are completely full, allowing the allocator to skip unnecessary searches 
	/// for available chunks, unlike the original Alexandrescu implementation.
	/// 
	/// This approach trades some space for potential gains 
	/// in allocation and deallocation speed.

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