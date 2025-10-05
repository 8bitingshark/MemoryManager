#include <cassert>
#include <cstdint> // uintptr_t
#include <stdexcept>
#include "CustomSmallObjAllocator\CtmFixedAllocator.h"
#include "SmallObjAllocator\SOA_defaults.h"
#include "SmallObjAllocator\SOA_debug.h"

/// -----------------------------------------------------------------------------
/// CtmFixedAllocator ctor
/// -----------------------------------------------------------------------------

soa::CtmFixedAllocator::CtmFixedAllocator(std::size_t blockSize)
	: m_blockSize(blockSize)
{
	assert(m_blockSize > 0);

	std::size_t numBlocks = DEFAULT_CHUNK_SIZE / blockSize;
	if (numBlocks > UCHAR_MAX) numBlocks = UCHAR_MAX;
	else if (numBlocks == 0) numBlocks = 8 * blockSize;

	m_numBlocks = static_cast<unsigned char>(numBlocks);

	assert(m_numBlocks == numBlocks);
}

/// -----------------------------------------------------------------------------
/// CtmFixedAllocator dtor
/// -----------------------------------------------------------------------------

soa::CtmFixedAllocator::~CtmFixedAllocator()
{
	SOA_LOG_OSS("Fixed Destructor");
	std::deque<Chunk>::iterator i = m_chunks.begin();
	for (; i != m_chunks.end(); ++i)
	{
		SOA_LOG_OSS("Chunk: blocks available: " << static_cast<int>(i->m_blocksAvailable));
		assert(i->m_blocksAvailable == m_numBlocks);
		i->Release();
	}

	m_chunks.clear();
	m_chunkMap.clear();
}

/// -----------------------------------------------------------------------------
/// CtmFixedAllocator move ctor
/// -----------------------------------------------------------------------------

soa::CtmFixedAllocator::CtmFixedAllocator(CtmFixedAllocator&& other) noexcept
	: m_blockSize(other.m_blockSize)
	, m_numBlocks(other.m_numBlocks)
	, m_chunks(std::move(other.m_chunks))
	, m_allocChunk(other.m_allocChunk)
	, m_deallocChunk(other.m_deallocChunk)
	, m_chunkMap(std::move(other.m_chunkMap))
{
	// steal approach
	other.m_allocChunk = nullptr;
	other.m_deallocChunk = nullptr;
	other.m_blockSize = 0;
	other.m_numBlocks = 0;
}

/// -----------------------------------------------------------------------------
/// CtmFixedAllocator move assignment
/// -----------------------------------------------------------------------------

soa::CtmFixedAllocator& soa::CtmFixedAllocator::operator=(CtmFixedAllocator&& other) noexcept
{
	if (this != &other) {
		
		m_blockSize = other.m_blockSize;
		m_numBlocks = other.m_numBlocks;
		m_chunks = std::move(other.m_chunks);
		m_allocChunk = other.m_allocChunk;
		m_deallocChunk = other.m_deallocChunk;
		m_chunkMap = std::move(other.m_chunkMap);

		other.m_allocChunk = nullptr;
		other.m_deallocChunk = nullptr;
		other.m_blockSize = 0;
		other.m_numBlocks = 0;
	}
	return *this;
}

/// -----------------------------------------------------------------------------
/// CtmFixedAllocator::Allocate
/// -----------------------------------------------------------------------------

void* soa::CtmFixedAllocator::Allocate()
{
	SOA_LOG_OSS("===BEGIN CtmFixedAllocator(" << this->m_blockSize << ")::Allocate===");
	SOA_LOG("Situation Before New Allocation: \n");

	SOA_PrintChunks(m_chunks);
	SOA_LOG("\n");
	SOA_PrintChunkMap(m_chunkMap);
	SOA_LOG("\n");
	SOA_LOG_OSS("numOfFullChunks: " << m_numFullChunks);


	// current allocChunk has no available blocks
	if (!m_allocChunk || m_allocChunk->m_blocksAvailable == 0)
	{
		// current chunk is full, so increment the number of full chunks
		if(!m_chunks.empty())
		{
			++m_numFullChunks;
		}
		
		SOA_LOG_OSS("numOfFullChunks++: " << m_numFullChunks << " | num of Chunks: " << m_chunks.size());
		assert(m_numFullChunks <= m_chunks.size());

		SOA_LOG_OSS("numOfFullChunks++: " << m_numFullChunks);

		// first checks if there is an empty free block for faster allocation
		// I think could improve "a little" in some scenarios, if you deallocate
		// like an entire chunk plus some of the others.
		//
		// Another strategy could be that you keep another vector of "quite empty chunks"
		// (for example chunks that has 2/3 of blocks available or also half of the blocks)
		// You pick always the first. When it is full you will move out from that vector
		//
		// I'm not sure that this second strategy is good for locality. If you do "sparse"
		// deallocations and then you have a bulk allocation and you try to fill all the 
		// empty little pieces, than if you deallocate all of them, you need to move
		// from a chunk to another

		if (!m_freeChunks.empty())
		{
			m_allocChunk = m_freeChunks.back();
			m_freeChunks.pop_back();

			// add the key to the map
			std::uintptr_t key = reinterpret_cast<uintptr_t>(m_allocChunk->m_pData);
			m_chunkMap[key] = m_allocChunk;
		}

		// find a suitable one 
		// if all of them are full, create a new chunk
		// 
		// An additional thing could be to store a variable that keeps track
		// about the number of fully occupied chunks, so here before searching with
		// the iterator you can test if the number of current chunks == to number of fully occupied
		//
		// This way could avoid an O(N) cost of going through all the chunks just to discover that 
		// all are occupied.

		else if(m_chunks.size() == m_numFullChunks)
		{
			// all full, allocate new Chunk
			
			m_chunks.emplace_back();
			Chunk* newChunkPtr = &m_chunks.back();
			newChunkPtr->Init(m_blockSize, m_numBlocks);

			// register to the map
			std::uintptr_t key = reinterpret_cast<uintptr_t>(newChunkPtr->m_pData);
			m_chunkMap[key] = newChunkPtr;

			// debug
			{
				SOA_LOG("CtmFixedAllocator - allocates a new chunk");
				SOA_LOG_OSS("m_pData points to address: " << reinterpret_cast<void*>(newChunkPtr->m_pData));
				SOA_LOG_OSS("new Chunk address: " << newChunkPtr);
				SOA_LOG("CtmFixedAllocator - newChunkPtr registered to the map");
				SOA_LOG("New map situation: ");
				SOA_LOG("\n");
				SOA_PrintChunkMap(m_chunkMap);
				SOA_LOG("\n");
			}


			m_allocChunk = newChunkPtr;
			m_deallocChunk = &m_chunks.front(); // m_deallocChunk = m_allocChunk; 
		}
		// some chunks are not full, so find the right one
		else 
		{
			for (auto& [base, chunk] : m_chunkMap) {
				if (chunk->m_blocksAvailable > 0) {
					m_allocChunk = chunk;
					break;
				}
			}
		}
	}

	assert(m_allocChunk);
	assert(m_allocChunk->m_blocksAvailable > 0);

	SOA_LOG("CtmFixedAllocator - forward allocation request to Chunk");

	SOA_LOG_OSS("===FINISH CtmFixedAllocator(" << this->m_blockSize << ")::Allocate===");
	return m_allocChunk->Allocate(m_blockSize);
}

/// -----------------------------------------------------------------------------
/// CtmFixedAllocator::Deallocate
/// -----------------------------------------------------------------------------
/// From cpp reference: https://en.cppreference.com/w/cpp/types/integer.html
/// uintptr_t: unsigned integer type capable of holding a pointer to void.

void soa::CtmFixedAllocator::Deallocate(void* p)
{
	SOA_LOG_OSS("===BEGIN CtmFixedAllocator(" << this->m_blockSize << ")::Deallocate===");

	assert(!m_chunks.empty());
	assert(&m_chunks.front() <= m_deallocChunk);
	assert(&m_chunks.back() >= m_deallocChunk);

	std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(p);

	// find chunk with upper_bound
	auto it = m_chunkMap.upper_bound(addr);
	if (it == m_chunkMap.begin()) {
		throw std::runtime_error("Pointer not in any chunk");
	}
	--it;

	Chunk* chunk = it->second;

	std::uintptr_t base = reinterpret_cast<std::uintptr_t>(chunk->m_pData);
	std::uintptr_t end = base + m_numBlocks * m_blockSize;

	assert(addr >= base && addr < end);

	m_deallocChunk = chunk;

	// debug
	SOA_LOG_OSS("p address:" << p);
	SOA_LOG_OSS("Chunk*: " << it->second);
	SOA_LOG("Current chunks before deallocation: ");
	SOA_PrintChunks(m_chunks);

	bool b_IsChunkFullBeforeDeallocation = m_deallocChunk->m_blocksAvailable == 0 ? true : false;
	SOA_LOG_OSS("Is ChunkFullBeforeDealloc: " << b_IsChunkFullBeforeDeallocation);

	DoDeallocate(p);

	if (b_IsChunkFullBeforeDeallocation)
	{
		--m_numFullChunks;
		SOA_LOG_OSS("FFFFFESFAEWFnumOfFullChunks--: " << m_numFullChunks);
	}

	// debug
	SOA_LOG("Current chunks after deallocation: ");
	SOA_PrintChunks(m_chunks);
	SOA_LOG_OSS("===FINISH CtmFixedAllocator(" << this->m_blockSize << ")::Deallocate===");
}

/// -----------------------------------------------------------------------------
/// CtmFixedAllocator::DoDeallocate
/// Performs deallocation. Assumes deallocChunk_ points to the correct chunk
/// -----------------------------------------------------------------------------
/// <summary>
/// Deallocates a memory block previously allocated by the CtmFixedAllocator 
/// and manages chunk release based on usage heuristics.
/// 
/// Rilasciare dopo 2 Chunk vuoti
/// Scopo: ridurre footprint e mantenere la cache locality 
/// senza accumulare troppi chunk vuoti.
/// 
/// Aumentare il numero di Chunk potrebbe migliorare per Bulk allocation grosse
/// dopo deallocazioni.
/// 
/// </summary>
/// <param name="p">Pointer to the memory block to deallocate.</param>

void soa::CtmFixedAllocator::DoDeallocate(void* p)
{
	SOA_LOG_OSS("===BEGIN CtmFixedAllocator(" << this->m_blockSize << ")::DoDeallocate===");

	assert(m_deallocChunk->m_pData <= p);
	assert(m_deallocChunk->m_pData + m_numBlocks * m_blockSize > p);

	SOA_LOG_OSS("CtmFixedAllocator (" << this->m_blockSize << ") - forward deallocation to chunk");

	m_deallocChunk->Deallocate(p, m_blockSize);

	// Check if the Chunk is empty
	if (m_deallocChunk->m_blocksAvailable == m_numBlocks)
	{
		// remove from map
		std::uintptr_t pt = reinterpret_cast<std::uintptr_t>(m_deallocChunk->m_pData);
		m_chunkMap.erase(pt);

		m_freeChunks.push_back(m_deallocChunk);
		m_deallocChunk = &m_chunks.front();
	}

	SOA_LOG_OSS("===FINISH CtmFixedAllocator(" << this->m_blockSize << ")::DoDeallocate===");
}