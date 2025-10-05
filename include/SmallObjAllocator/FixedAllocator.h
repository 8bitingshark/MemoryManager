#ifndef FIXED_ALLOCATOR_H
#define FIXED_ALLOCATOR_H

#include <vector>
#include "Chunk.h"

namespace soa {

	/// FixedAllocator from Modern C++ Design by Alexandrescu
	/// 
	/// - Uses Chunk as a building block.
	/// - Aggregates an array of Chunks.
	/// - Primary purposes: satisfy memory requests that go beyond a
	///   Chunk's capacity.
	/// - All existing Chunks occupied? New Chunk appended to array.
	/// - Forward request to Chunk.
	/// 
	/// Capacity is limited by available memory.
	/// 
	/// Allocation strategy - for a faster lookup:
	/// - It holds a pointer to the last chunk that was used for an allocation.
	/// - First check to the allocChunk availability.
	/// - Ok? Alloc satisfied presto
	/// - If not, a linear search occurs
	/// - In any case, allocChunk is updated to point to that found or added
	///   chunk. This way we increase the likelihood of a fast allocation next time.
	/// 
	/// Deallocation strategy: (pg.90-91)
	/// - Cache system seems bad with Bulk allocation trends and dealloc in order/reverse
	///   order. It works well with Bufferfly trend.
	/// - For those better solution is adopting a similar strategy of allocChunk.
	/// - Keep the pointer to the last Chunk object that was used for a deallocation.
	/// - New Deallocation request: first search into deallocChunk
	/// - No? Linear search.
	/// 
	/// Improvements tweaks:
	/// 1) Deallocate searches the appropriate Chunk starting from deallocChunk_'s
	///    vicinity: going up and down from deallocChunk with 2 iterators (faster)
	///     - This greatly improves the speed of bulk deallocations in any 
	///       order (normal or reversed).
	/// 2) Avoid constantly allocate/deallocate a new Chunk. During deallocation, 
	///    a chunk is freed only when there are two empty chunks.
	///     - This heuristic isn't perfect. If in a loop you allocate a bunch of
	///       smart pointers of an appropriate size, you end un probably allocated
	///       2 chunks and at the end deallocatin.
	///     
	/// The deallocation strategy chosen also fits the butterfly allocation trend acceptably. 
	/// Even if not allocating data in an ordered manner, 
	/// programs tend to foster a certain locality; that is, 
	/// they access a small amount of data at a time.

	class FixedAllocator {

		std::size_t m_blockSize{};
		unsigned char m_numBlocks{};
		std::vector<Chunk> m_chunks;
		Chunk* m_allocChunk = nullptr;
		Chunk* m_deallocChunk = nullptr;

		mutable const FixedAllocator* m_prev{};
		mutable const FixedAllocator* m_next{};

		void DoDeallocate(void* p);
		Chunk* VicinityFind(void* p);

	public:

		explicit FixedAllocator(std::size_t blockSize = 0);
		FixedAllocator(const FixedAllocator&);
		FixedAllocator& operator=(const FixedAllocator&);
		~FixedAllocator();

		void Swap(FixedAllocator& rhs);

		void* Allocate();
		void  Deallocate(void* p);
		inline std::size_t GetBlockSize() const { return m_blockSize; }
	};
}

#endif // !FIXED_ALLOCATOR_H
