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
	// current allocChunk has no available blocks
	if (!m_allocChunk || m_allocChunk->m_blocksAvailable == 0)
	{
		// current chunk is full, so increment the number of full chunks
		if(!m_chunks.empty())
		{
			++m_numFullChunks;
		}
		
		assert(m_numFullChunks <= m_chunks.size());

		// first checks if there is an empty free block for faster allocation
		// I think could improve "a little" in some scenarios, if you deallocate
		// completely some chunks at a some point. But obviously it depends from
		// real case scenarios. For now, I just like to think and take different
		// approaches for exercise.

		if (!m_freeChunks.empty())
		{
			m_allocChunk = m_freeChunks.back();
			m_freeChunks.pop_back();

			// add the key to the map
			std::uintptr_t key = reinterpret_cast<uintptr_t>(m_allocChunk->m_pData);
			m_chunkMap[key] = m_allocChunk;
		}

		// no empty chunks? Check if all of them are full, if that so
		// immediately create a new chunk.

		else if(m_chunks.size() == m_numFullChunks)
		{
			// all full, allocate new Chunk
			
			m_chunks.emplace_back();
			Chunk* newChunkPtr = &m_chunks.back();
			newChunkPtr->Init(m_blockSize, m_numBlocks);

			// register to the map
			std::uintptr_t key = reinterpret_cast<uintptr_t>(newChunkPtr->m_pData);
			m_chunkMap[key] = newChunkPtr;

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

	return m_allocChunk->Allocate(m_blockSize);
}

/// -----------------------------------------------------------------------------
/// CtmFixedAllocator::Deallocate
/// -----------------------------------------------------------------------------
/// From cpp reference: https://en.cppreference.com/w/cpp/types/integer.html
/// uintptr_t: unsigned integer type capable of holding a pointer to void.

void soa::CtmFixedAllocator::Deallocate(void* p)
{
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

	bool b_IsChunkFullBeforeDeallocation = m_deallocChunk->m_blocksAvailable == 0 ? true : false;

	DoDeallocate(p);

	// if the chunk was full decrease m_numFullChunks
	if (b_IsChunkFullBeforeDeallocation)
	{
		--m_numFullChunks;
	}
}

/// -----------------------------------------------------------------------------
/// CtmFixedAllocator::DoDeallocate
/// -----------------------------------------------------------------------------
/// Performs deallocation. Assumes deallocChunk_ points to the correct chunk
/// 
/// For now just checks if the chunk is empty, remove for map and add it to
/// the vector of empty chunks

void soa::CtmFixedAllocator::DoDeallocate(void* p)
{
	assert(m_deallocChunk->m_pData <= p);
	assert(m_deallocChunk->m_pData + m_numBlocks * m_blockSize > p);

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
}