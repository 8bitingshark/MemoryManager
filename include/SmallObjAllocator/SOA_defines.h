#ifndef SOA_DEFINES_H
#define SOA_DEFINES_H

#include <new>
#include <limits>
#include <utility>
#include "SOA_defaults.h"
#include "SmallObjAllocator.h"

namespace soa {

	// C-style functions 

	inline void* soa_malloc(std::size_t n) {
		if (n == 0) return nullptr;
		return SmallObjAllocator::Instance().Allocate(n);
	}

	inline void soa_free(void* p, std::size_t n) {
		if (!p) return;

		SmallObjAllocator::Instance().Deallocate(p, n);
		return;
	}

	// C++ functions
	
	template<typename T>
	T* soa_new(const std::nothrow_t&) noexcept
	{
		void* p = soa_malloc(sizeof(T));
		if (!p) return nullptr;
		return new(p) T();
	}

	template<typename T>
	T* soa_new()
	{
		if (T* p = soa_new<T>(std::nothrow)) return p;
		throw std::bad_alloc();
	}

	template<typename T, typename... Args>
	T* soa_new(const std::nothrow_t&, Args&&... args) noexcept
	{
		void* p = soa_malloc(sizeof(T));
		if (!p) return nullptr;

		T* tp = reinterpret_cast<T*>(p);
		return new(tp) T(std::forward<Args>(args)...);  // std::forward previene conversione/copia passando un l-value
	}

	template<typename T, typename... Args>
	T* soa_new(Args&&... args)
	{
		if (T* p = soa_new<T>(std::nothrow, std::forward<Args>(args)...)) return p;
		throw std::bad_alloc();
	}

	template<typename T>
	void soa_delete(void* p)
	{
		if (!p) return;

		p->~T();
		return soa_free(p, sizeof(T));
	}

	template<typename T, typename... Args>
	T* soa_new_array(const std::nothrow_t&, const std::size_t count, Args&&... args) noexcept
	{
		if (count == 0) return nullptr;

		const std::size_t max_count =
			(std::numeric_limits<std::size_t>::max() - sizeof(count)) / sizeof(T);

		if (count > max_count) return nullptr;

		const std::size_t total_size = count * sizeof(T);
		void* p{};

		// store also count at the beginning of the array
		p = soa_malloc(total_size + sizeof(count));
		std::size_t* countPtr = reinterpret_cast<std::size_t*>(p);
		*countPtr = count;

		// construct array elements
		T* array = reinterpret_cast<T*>(countPtr + 1);
		for (std::size_t i = 0; i < count; ++i)
		{
			try {
				new(&array[i]) T(std::forward<Args>(args)...);
			}
			catch (...)
			{
				// unroll construction and release memory
				for (std::size_t j = 0; j < i; ++j)
				{
					array[j].~T();
				}
				soa_free(countPtr, total_size + sizeof(count)); // deallocate all
				return nullptr;
			}
		}

		return array;
	}

	template<typename T, typename... Args>
	T* soa_new_array(const std::size_t count, Args&&... args)
	{
		if (count == 0) return nullptr;

		const std::size_t max_count =
			(std::numeric_limits<std::size_t>::max() - sizeof(count)) / sizeof(T);

		if (count > max_count) return nullptr;

		const std::size_t total_size = count * sizeof(T);
		void* p{};

		// store also size at the beginning of the array
		p = soa_malloc(total_size + sizeof(count));
		std::size_t* sizePtr = reinterpret_cast<std::size_t*>(p);
		*sizePtr = total_size;

		// construct array elements
		T* array = reinterpret_cast<T*>(sizePtr + 1);
		for (std::size_t i = 0; i < count; ++i)
		{
			new(&array[i]) T(std::forward<Args>(args)...);
		}
		return array;
	}

	template<typename T>
	void soa_delete_array(void* p)
	{
		if (!p) return true;

		std::size_t* size_ptr = reinterpret_cast<std::size_t*>(p) - 1;
		const std::size_t size = *size_ptr;
		const std::size_t count = size / sizeof(T);

		for (std::size_t i = 0; i < count; ++i) p[i].~T();

		return soa_free(size_ptr, sizeof(size) + size);
	}
}

#endif // !SOA_DEFINES_H
