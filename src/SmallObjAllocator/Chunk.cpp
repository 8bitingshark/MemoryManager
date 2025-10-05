#include <cassert>
#include <cstdlib>
#include "SmallObjAllocator\Chunk.h"
#include "SmallObjAllocator\SOA_debug.h"

/// -----------------------------------------------------------------------------
/// FixedAllocator::Chunk::Init
/// -----------------------------------------------------------------------------
/// Initializes a chunk object

void soa::Chunk::Init(std::size_t blockSize, unsigned char blocks)
{
	assert(blockSize > 0);
	assert(blocks > 0);
	assert((blockSize * blocks) / blockSize == blocks);

	SOA_LOG_OSS("Chunk (" << blockSize << " (blockSize), " << static_cast<int>(blocks) << " (blocks)) - Init");
	SOA_LOG_OSS("Chunk total Size: " << blockSize * blocks);

	m_pData = static_cast<unsigned char*>(std::malloc(blockSize * blocks)); // m_pData = new unsigned char[blockSize * blocks];

	Reset(blockSize, blocks);
}

/// -----------------------------------------------------------------------------
/// FixedAllocator::Chunk::Allocate
/// -----------------------------------------------------------------------------
/// Allocates a block from a chunk.
/// Since in m_firstAvailableBlock is stored the index of the next available
/// block, when you allocate a new one, you can update the next available
/// using the index stored in the previous available (the current).

void* soa::Chunk::Allocate(std::size_t blockSize)
{
	if (!m_blocksAvailable)
		return nullptr;

	assert((m_firstAvailableBlock * blockSize) / blockSize ==
		m_firstAvailableBlock);

	unsigned char* pResult = m_pData + m_firstAvailableBlock * blockSize;

	m_firstAvailableBlock = *pResult;
	--m_blocksAvailable;

	return pResult;
}

/// -----------------------------------------------------------------------------
/// FixedAllocator::Chunk::Deallocate
/// -----------------------------------------------------------------------------
/// Deallocates a block from a chunk

void soa::Chunk::Deallocate(void* p, std::size_t blockSize)
{
	assert(p >= m_pData);

	unsigned char* pToRelease = static_cast<unsigned char*>(p);

	assert((pToRelease - m_pData) % blockSize == 0);

	*pToRelease = m_firstAvailableBlock;
	m_firstAvailableBlock = static_cast<unsigned char>(
		(pToRelease - m_pData) / blockSize);

	assert(m_firstAvailableBlock == (pToRelease - m_pData) / blockSize);

	++m_blocksAvailable;
}

/// -----------------------------------------------------------------------------
/// FixedAllocator::Chunk::Reset
/// -----------------------------------------------------------------------------
/// Clears an already allocated chunk and init the free list

void soa::Chunk::Reset(std::size_t blockSize, unsigned char blocks)
{
	m_firstAvailableBlock = 0;
	m_blocksAvailable = blocks;

	unsigned char i = 0;
	unsigned char* p = m_pData;
	for (; i != blocks; p += blockSize)
	{
		*p = ++i;
	}
}

/// -----------------------------------------------------------------------------
/// FixedAllocator::Chunk::Release
/// -----------------------------------------------------------------------------
/// Releases the data managed by a chunk

void soa::Chunk::Release()
{
	SOA_LOG_OSS("Chunk - Release");
	SOA_LOG_OSS("Chunk: blocksAvailable: " << static_cast<int>(m_blocksAvailable));
	
	std::free(m_pData); //delete[] m_pData;
}