#ifndef ALLOC_TYPEDEF_H
#define ALLOC_TYPEDEF_H

#include "mema\STL_Allocator.h"
#include "mema\SystemBackend.h"
#include "mema\SoaBacked.h"

namespace mema {

	template<typename T>
	using SystemAllocatorSTL = STLAllocator<T, SystemBackend>;

	template<typename T>
	using SoaAllocatorSTL = STLAllocator<T, SOABackend>;
}

#endif // !ALLOC_TYPEDEF_H
