#ifndef SOA_OVERRIDES_H
#define SOA_OVERRIDES_H

#include <cassert>
#include "SOA_defines.h"

#ifdef USE_SMALL_OBJ_ALLOC

void* operator new(std::size_t n) {
	if (void* p = soa::soa_malloc(n)) return p;
	throw std::bad_alloc();
}

void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
	return soa::soa_malloc(n);
}

// from C++14
void operator delete(void* p, std::size_t n) noexcept 
{ 
	soa::soa_free(p, n);
}

void* operator new[](std::size_t size) {
	if (void* p = ::operator new[](size, std::nothrow)) return p;
	throw std::bad_alloc();
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
	void* p = soa::soa_malloc(size + sizeof(std::size_t));
	if (!p) return nullptr;

	std::size_t* size_ptr = reinterpret_cast<std::size_t*>(p);
	*size_ptr = size;
	return size_ptr + 1;
}


void operator delete[](void* p) noexcept
{
	if (!p) return;

	std::size_t* size_ptr = reinterpret_cast<std::size_t*>(p) - 1;
	std::size_t size = *size_ptr;

	soa::soa_free(size_ptr, sizeof(std::size_t) + size);
}

void operator delete[](void* p, std::size_t n) noexcept {
	if (!p) return;
	
	// free also the header with the size
	std::size_t* const size_ptr = reinterpret_cast<std::size_t*>(p) - 1;
	assert(n == *size_ptr);

	soa::soa_free(reinterpret_cast<std::size_t*>(p) - 1,
		sizeof(std::size_t) + n);
}

#endif !USE_SMALL_OBJ_ALLOC

#endif !SOA_OVERRIDES_H