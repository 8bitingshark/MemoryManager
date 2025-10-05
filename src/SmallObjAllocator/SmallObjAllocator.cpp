#include <cassert>
#include "SmallObjAllocator\SmallObjAllocator.h"
#include "SmallObjAllocator\SOA_debug.h"

/// -----------------------------------------------------------------------------
/// SmallObjAllocator::SmallObjAllocator ctor
/// -----------------------------------------------------------------------------

soa::SmallObjAllocator::SmallObjAllocator(std::size_t chunkSize, std::size_t maxObjectSize)
	: m_chunkSize(chunkSize), m_maxObjSize(maxObjectSize)
{
}

/// -----------------------------------------------------------------------------
/// Functor
/// -----------------------------------------------------------------------------

namespace {

	struct CompareFixedAllocatorSize {

		bool operator()(const soa::FixedAllocator& x, std::size_t numBytes) const noexcept {
			return x.GetBlockSize() < numBytes;
		}
	};
}

/// -----------------------------------------------------------------------------
/// SmallObjAllocator::Allocate
/// -----------------------------------------------------------------------------
/// - If the size of the request is greater then the max small object size handled
///   fallback to the default allocator.
/// - First checks if the last fixed allocator used for dealloc has the 
///   right block size. In that case forward allocation request.
/// - Otherwise linear search.

void* soa::SmallObjAllocator::Allocate(std::size_t numBytes)
{
	if (numBytes > m_maxObjSize)
	{
		SOA_LOG("std::malloc called");
		return std::malloc(numBytes); // previous: return operator new(numBytes); Bad with global overrides
	}
		

	if (m_pLastAlloc && m_pLastAlloc->GetBlockSize() == numBytes)
	{
		SOA_LOG("Soa allocate called");
		return m_pLastAlloc->Allocate();
	}

	std::vector<FixedAllocator>::iterator it = std::lower_bound(
		m_Pool.begin(),
		m_Pool.end(),
		numBytes,
		CompareFixedAllocatorSize{});

	if (it == m_Pool.end() || it->GetBlockSize() != numBytes)
	{
		it = m_Pool.insert(it, FixedAllocator(numBytes));
		m_pLastDealloc = &*m_Pool.begin();
	}

	m_pLastAlloc = &*it;

	SOA_LOG("Soa allocate called");
	return m_pLastAlloc->Allocate();
}

/// -----------------------------------------------------------------------------
/// SmallObjAllocator::Deallocate
/// -----------------------------------------------------------------------------
/// Delegate to the user remember the actual size for deallocation to improve
/// speed searching.

void soa::SmallObjAllocator::Deallocate(void* p, std::size_t numBytes)
{
	if (numBytes > m_maxObjSize)
	{
		SOA_LOG("std::free called");
		return std::free(p);
	}
		
	if (m_pLastDealloc && m_pLastDealloc->GetBlockSize() == numBytes)
	{
		SOA_LOG("Soa deallocate called");
		m_pLastDealloc->Deallocate(p);
		return;
	}

	std::vector<FixedAllocator>::iterator it = std::lower_bound(
		m_Pool.begin(),
		m_Pool.end(),
		numBytes,
		CompareFixedAllocatorSize{});

	assert(it != m_Pool.end());
	assert(it->GetBlockSize() == numBytes);

	m_pLastDealloc = &*it;

	SOA_LOG("Soa deallocate called");
	m_pLastDealloc->Deallocate(p);
}