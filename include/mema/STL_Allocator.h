#ifndef STL_ALLOCATOR_H
#define STL_ALLOCATOR_H

#include <cstddef>
#include <memory>
#include <type_traits>
#include <new>
#include <cassert>

namespace mema {

	template<typename T, typename AllocBackend>
	class STLAllocator {

	public:

		using value_type = T;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using const_pointer = const T*;

		template<typename U>
		struct rebind { using other = STLAllocator<U, AllocBackend>; };
		
		STLAllocator() noexcept = default;
		//STLAllocator(const STLAllocator&) noexcept = default;

		template<typename U>
		STLAllocator(const STLAllocator<U, AllocBackend>&) noexcept {}

		pointer allocate(size_type n) {
			void* ptr = AllocBackend::Allocate(n * sizeof(T));
			if (!ptr) throw std::bad_alloc();
			return static_cast<pointer>(ptr);
		}

		void deallocate(pointer p, size_type n) noexcept {
			AllocBackend::Free(p, n * sizeof(T));
		}

		// max size
		size_type max_size() const noexcept {
			return (std::numeric_limits<size_type>::max)() / sizeof(T);
		}

		// equality operators (templated)
		template<typename U>
		bool operator==(const STLAllocator<U, AllocBackend>&) const noexcept { return true; }

		template<typename U>
		bool operator!=(const STLAllocator<U, AllocBackend>& other) const noexcept { return !(*this == other); }
	};
}


#endif // !STL_ALLOCATOR_H