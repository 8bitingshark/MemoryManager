#include "SmallObjAllocator\SmallObject.h"
#include "SmallObjAllocator\SmallObjAllocator.h"

/// -----------------------------------------------------------------------------
/// SmallObject::operator new
/// ----------------------------------------------------------------------------- 

void* soa::SmallObject::operator new(const std::size_t smallObjSize)
{
	void* const p = SmallObjAllocator::Instance().Allocate(smallObjSize);
	if (!p) throw std::bad_alloc();
	return p;
}

/// -----------------------------------------------------------------------------
/// SmallObject::operator delete
/// ----------------------------------------------------------------------------- 

void soa::SmallObject::operator delete(void* const p, const std::size_t size) noexcept
{
	SmallObjAllocator::Instance().Deallocate(p, size);
}

