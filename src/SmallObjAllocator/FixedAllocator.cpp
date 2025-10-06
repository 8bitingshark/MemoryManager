#include <cassert>
#include "SmallObjAllocator\SOA_defaults.h"
#include "SmallObjAllocator\FixedAllocator.h"
#include "SmallObjAllocator\SOA_debug.h"

/// -----------------------------------------------------------------------------
/// FixedAllocator explicit ctor
/// -----------------------------------------------------------------------------

soa::FixedAllocator::FixedAllocator(std::size_t blockSize)
	: m_blockSize(blockSize)
{
	assert(m_blockSize > 0);

	m_prev = m_next = this;

	std::size_t numBlocks = DEFAULT_CHUNK_SIZE / blockSize;
	if (numBlocks > UCHAR_MAX) numBlocks = UCHAR_MAX;
	else if (numBlocks == 0) numBlocks = 8 * blockSize;

	m_numBlocks = static_cast<unsigned char>(numBlocks);

	assert(m_numBlocks == numBlocks);
}

/// -----------------------------------------------------------------------------
/// FixedAllocator copy ctor
/// -----------------------------------------------------------------------------
/// Uses a double linked list system to prevent releasing resources
/// when dtor is called, if a FixedAllocator is copied

soa::FixedAllocator::FixedAllocator(const FixedAllocator& i_other)
	: m_blockSize(i_other.m_blockSize)
	, m_numBlocks(i_other.m_numBlocks)
	, m_chunks(i_other.m_chunks)
{
	m_prev = &i_other;
	m_next = i_other.m_next;

	i_other.m_next->m_prev = this;
	i_other.m_next = this;

	m_allocChunk = i_other.m_allocChunk
		? &m_chunks.front() + (i_other.m_allocChunk - &i_other.m_chunks.front())
		: nullptr;

	m_deallocChunk = i_other.m_deallocChunk
		? &m_chunks.front() + (i_other.m_deallocChunk - &i_other.m_chunks.front())
		: nullptr;
}

/// -----------------------------------------------------------------------------
/// FixedAllocator assignment operator
/// -----------------------------------------------------------------------------

soa::FixedAllocator& soa::FixedAllocator::operator=(const FixedAllocator& i_other)
{
	FixedAllocator copy(i_other);
	copy.Swap(*this);
	return *this;
}

/// -----------------------------------------------------------------------------
/// FixedAllocator dtor
/// -----------------------------------------------------------------------------
/// It prevents memory deallocation if the FixedAllocator is copied

soa::FixedAllocator::~FixedAllocator()
{
	if (m_prev != this)
	{
		m_prev->m_next = m_next;
		m_next->m_prev = m_prev;
		return;
	}

	assert(m_prev == m_next);

	std::vector<Chunk>::iterator i = m_chunks.begin();
	for (; i != m_chunks.end(); ++i)
	{
		assert(i->m_blocksAvailable == m_numBlocks);
		i->Release();
	}
}

/// -----------------------------------------------------------------------------
/// FixedAllocator::Swap
/// -----------------------------------------------------------------------------

void soa::FixedAllocator::Swap(FixedAllocator& rhs)
{
	using namespace std;
	swap(m_blockSize, rhs.m_blockSize);
	swap(m_numBlocks, rhs.m_numBlocks);
	m_chunks.swap(rhs.m_chunks);
	swap(m_allocChunk, rhs.m_allocChunk);
	swap(m_deallocChunk, rhs.m_deallocChunk);
}

/// -----------------------------------------------------------------------------
/// FixedAllocator::Allocate
/// 
/// -----------------------------------------------------------------------------
/// [NOTE]
/// Some memory allocation trends make this scheme
/// work inefficiently; however, they tend not to appear often in practice.
/// Don't forget that every allocator has
/// an Achilles' heel.

void* soa::FixedAllocator::Allocate()
{
	if (!m_allocChunk || m_allocChunk->m_blocksAvailable == 0)
	{
		std::vector<Chunk>::iterator it = m_chunks.begin();

		for (;; ++it)
		{
			// allocate new chunk
			if (it == m_chunks.end())
			{
				m_chunks.reserve(m_chunks.size() + 1);
				Chunk newChunk;
				newChunk.Init(m_blockSize, m_numBlocks);
				m_chunks.push_back(newChunk); // copy

				m_allocChunk = &m_chunks.back();
				m_deallocChunk = m_allocChunk;   // &m_chunks.front();
				break;
			}

			// found an usable chunk
			if (it->m_blocksAvailable > 0)
			{
				m_allocChunk = &*it;
				break;
			}
		}
	}

	assert(m_allocChunk);
	assert(m_allocChunk->m_blocksAvailable > 0);

	return m_allocChunk->Allocate(m_blockSize);
}

/// -----------------------------------------------------------------------------
/// FixedAllocator::Deallocate
/// -----------------------------------------------------------------------------
/// Deallocates a block previously allocated with Allocate
/// (undefined behavior if called with the wrong pointer)

void soa::FixedAllocator::Deallocate(void* p)
{
	assert(!m_chunks.empty());
	assert(&m_chunks.front() <= m_deallocChunk);
	assert(&m_chunks.back() >= m_deallocChunk);

	m_deallocChunk = VicinityFind(p);

	assert(m_deallocChunk);

	DoDeallocate(p);
}

/// -----------------------------------------------------------------------------
/// FixedAllocator::VicinityFind 
/// -----------------------------------------------------------------------------

soa::Chunk* soa::FixedAllocator::VicinityFind(void* p)
{
	assert(!m_chunks.empty());
	assert(m_deallocChunk);

	const std::size_t chunkLength = m_blockSize * m_numBlocks;

	Chunk* low = m_deallocChunk;
	Chunk* high = m_deallocChunk + 1;
	Chunk* lowBound = &m_chunks.front();
	Chunk* highBound = &m_chunks.back() + 1;

	// special case, deallocChunk last in vector
	if (high == highBound) high = nullptr;

	for (;;)
	{
		if (low)
		{
			// check if p is in the current chunk evaluated
			if (p >= low->m_pData && p < low->m_pData + chunkLength)
			{
				return low;
			}
			if (low == lowBound) low = nullptr;
			else --low;
		}

		if (high)
		{
			if (p >= high->m_pData && p < high->m_pData + chunkLength)
			{
				return high;
			}
			if (++high == highBound) high = nullptr;
		}
	}

	assert(false);
	return nullptr;
}

/// -----------------------------------------------------------------------------
/// FixedAllocator::DoDeallocate
/// -----------------------------------------------------------------------------
/// Performs deallocation. Assumes deallocChunk_ points to the correct chunk
/// Heuristic: when we have two empty chunks, realese one

void soa::FixedAllocator::DoDeallocate(void* p)
{
	assert(m_deallocChunk->m_pData <= p);
	assert(m_deallocChunk->m_pData + m_numBlocks * m_blockSize > p);

	m_deallocChunk->Deallocate(p, m_blockSize);

	// check if we need release it
	if (m_deallocChunk->m_blocksAvailable == m_numBlocks)
	{
		Chunk& lastChunk = m_chunks.back();

		if (&lastChunk == m_deallocChunk)
		{
			if (m_chunks.size() > 1 &&
				m_deallocChunk[-1].m_blocksAvailable == m_numBlocks)
			{
				// two chunks empty
				lastChunk.Release();
				m_chunks.pop_back();
				m_allocChunk = m_deallocChunk = &m_chunks.front();
			}
			return;
		}

		if (lastChunk.m_blocksAvailable == m_numBlocks)
		{
			// two empty
			lastChunk.Release();
			m_chunks.pop_back();
			m_allocChunk = m_deallocChunk;
		}
		else
		{
			// we want empties to the end
			std::swap(*m_deallocChunk, lastChunk);
			m_allocChunk = &m_chunks.back(); // empty, so ready for new allocations
		}
	}
}