#ifndef SMALL_OBJECT_H
#define SMALL_OBJECT_H

#include <cstddef>

namespace soa {

	/// SmallObject
	///
	/// - wraps SmallObjAllocator to offer encapsulated allocation services for C++
	///   classes.
	/// - Overloads operator new and operator delete and passes them to a
	///	  SmallObjAllocator object. 
	///   This way, you make your objects benefit from specialized allocation by
	///	  simply deriving them from SmallObject.

	class SmallObject {

		static void* operator new(const std::size_t smallObjSize);
		static void operator delete(void* const p, const std::size_t size) noexcept;

		virtual ~SmallObject() {}
	};
}

#endif // !SMALL_OBJECT_H
