#ifndef CHUNK_H
#define CHUNK_H

#include <cstddef>

namespace soa {

	/// Chunk from Modern C++ Design by Alexandrescu
	/// 
	/// - POD (Plain Old Data)
	/// - Contains and manages a chunk of memory consisting 
	///   of an integral number of fixed size
	///   blocks.
	/// - Allows to allocate and deallocate memory blocks.
	/// - At construction time, you configure the block size and the number of blocks.
	/// - When there are no more blocks available in the chunk, 
	///   the allocation function fails by returning zero.
	/// 
	/// Chunk doesn't hold blockSize. You'll need to pass it.
	/// blockSize it is known at superior level.
	/// Not stored here to avoid waste space and time.
	/// For efficiency reason no ctor, dtor, assignment operators.
	/// Proper copy semantics here hurts efficiency to upper layers.
	/// 
	/// [*] A block can be either used or unused. We can store whatever we want in an
	///     unused block, so we take advantage of this.
	/// [*] The first byte of an unused block holds the index of the next
	///     unused block.
	/// 
	/// Because we hold the first available index in firstAvailableBlock_, 
	/// we have a fullfledged singly linked list 
	/// of unused blocks without using any extra memory.
	/// - No extra cost in size.
	/// - Allocating and deallocating a block inside 
	///   a Chunk takes constant time.

	struct Chunk {

		void  Init(std::size_t i_blockSize, unsigned char blocks);
		void* Allocate(std::size_t blockSize);
		void  Deallocate(void* p, std::size_t blockSize);
		void  Reset(std::size_t blockSize, unsigned char blocks);
		void  Release();

		unsigned char* m_pData{};
		unsigned char  m_firstAvailableBlock{};
		unsigned char  m_blocksAvailable{};
	};
}

#endif // !CHUNK_H