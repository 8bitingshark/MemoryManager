#ifndef MSTL_VECTOR_H
#define MSTL_VECTOR_H

#include <initializer_list>
#include <vector>
#include <algorithm>
#include <iostream>

// concepts
template<typename E>
concept Element = std::semiregular<E>;

template<typename N>
concept Number = std::integral<N> || std::floating_point<N>;

template<typename T>
concept Boolean = std::convertible_to<T, bool>;

namespace mstl {

	///
	/// vector_rep
	/// 
	
	template<typename T, typename A = std::allocator<T>>
	struct vector_rep {

		using alloc_traits = std::allocator_traits<A>;
		using size_type = typename alloc_traits::size_type;  // tipically: std::size_t

		[[no_unique_address]] A alloc{}; //from C++20
		size_type sz{};
		T* elem{};    //pImpl idiom
		size_type space{};

		vector_rep() : alloc{ A{} }, sz{ 0 }, elem{ nullptr }, space{ 0 } {}
		vector_rep(const A& in_allocator) : alloc{ in_allocator }, sz{ 0 }, elem{ nullptr }, space{ 0 } {}
		vector_rep(const A& in_allocator, size_type n)
			: alloc{ in_allocator }, sz{ 0 }, elem{ alloc_traits::allocate(alloc, n) }, space{ n } {
			std::cout << "ctor vector_rep called\n";
		}
		~vector_rep() {
			std::cout << "dtor vector_rep called\n";
			alloc_traits::deallocate(alloc, elem, space);
		}
	};

	/// 
	/// STL vector implementation
	/// using alloc_traits
	/// 

	template<typename T, typename A = std::allocator<T>>
		requires Element<T>
	class vector {

	private:
		vector_rep<T, A> r;
		using alloc_traits = typename vector_rep<T, A>::alloc_traits;

	public:
		using size_type = typename vector_rep<T, A>::size_type;
		using value_type = T;
		using iterator = T*;
		using const_iterator = const T*;

		vector() : r{} { std::cout << "vector ctor called\n"; }

		explicit vector(size_type i_size, value_type def = value_type{})
			: r{ A{}, i_size }
		{
			std::cout << "vector ctor called\n";

			if (!(i_size >= 0 && i_size < reasonable_size))
				throw std::length_error{ "Wrong size for vector" };

			resize(i_size, def);
		}

		vector(std::initializer_list<T> i_lst)
			: r{ A{}, static_cast<size_type>(i_lst.size()) }
		{
			std::cout << "vector ctor called\n";
			if (capacity() > 0)
			{
				int i = 0;
				for (const T& v : i_lst) {
					alloc_traits::construct(alloc, &r.elem[i++], v);
				}

				r.sz = static_cast<size_type>(i_lst.size());
			}
		}

		// copy constructor
		vector(const vector& i_v)
			: r{ i_v.r.alloc, i_v.r.sz }
		{
			// if construction fails, it is nice to destroy objects 
			// inside a catch block

			if (i_v.size() > 0) {
				size_type i = 0;
				for (const T& v : i_v) {
					alloc_traits::construct(r.alloc, &r.elem[i++], v);
				}
				r.sz = i_v.size();
			}
		}

		// copy assignment
		vector& operator=(const vector& i_v)
		{
			if (this == &i_v) // self-assignment check
				return *this;

			if (i_v.size() <= size())
			{
				// since i_v is a const ref, move algo make copies and does not move
				std::move(i_v.begin(), i_v.begin() + i_v.size(), begin());
				std::destroy(begin() + i_v.size(), end());		// destroy eventual surplus
			}

			// need more space

			vector tmp(i_v);      // if this fails this and i_v remains intact (probably I need to destroy the partially copied tmp?)
			swap(*this, tmp);     // strong guarantee
			return *this;
		}

		// move constructor
		vector(vector&& i_v) noexcept
			: r{ i_v.r.alloc }
		{
			r.sz = i_v.size();
			r.space = i_v.capacity();
			r.elem = i_v.r.elem;

			i_v.r.sz = i_v.r.space = 0;
			i_v.r.elem = nullptr;
		}

		// move assignment
		/*
		Move assignment operators typically transfer the resources held by the argument
		(e.g. pointers to dynamically-allocated objects, file descriptors, TCP sockets...),
		rather than make copies of them, and leave the argument
		in some valid but otherwise indeterminate state.
		Since move assignment doesn’t change the lifetime of the argument,
		the destructor will typically be called on the argument at a later point.
		For example, move-assigning from a std::string or from a std::vector
		may result in the argument being left empty.
		A move assignment is less, not more restrictively defined than ordinary assignment;
		where ordinary assignment must leave two copies of data at completion,
		move assignment is required to leave only one.
		*/
		vector& operator=(vector&& i_v) noexcept
		{
			// here simply swaps with no "steal-and-null-other" strategy
			if (this != &i_v) {
				swap(*this, i_v);
			}
			return *this;
		}

		// destructor
		~vector()
		{
			std::cout << "vector dtor called\n";

			for (size_type i = 0; i < r.sz; ++i) {
				alloc_traits::destroy(r.alloc, &r.elem[i]);
			}

			// deallocation managed by vector_rep
		}

		// checked accesses
		T& at(size_type n)
		{
			if (n < 0 || size() <= n)
				throw out_of_range_bs(n);

			return r.elem[n];
		}
		const T& at(size_type n) const
		{
			if (n < 0 || size() <= n)
				throw out_of_range_bs(n);

			return r.elem[n];
		}

		// unchecked access
		// access for non-const vectors
		value_type& operator[](size_type n) { return r.elem[n]; }
		// access for const vectors
		const value_type& operator[](size_type n) const { return r.elem[n]; }

		// get size
		size_type size() const { return r.sz; }
		// get capacity
		size_type capacity() const { return r.space; }

		// growth: reserve, resize, push_back
		void reserve(size_type newAlloc)
		{
			if (newAlloc <= capacity())
				return;

			vector_rep<T, A> b{ r.alloc, newAlloc };
			
			//std::uninitialized_move(begin(), end(), b.elem);
			//std::destroy(begin(), end());
			for (size_type i = 0; i < r.sz; ++i) {
				alloc_traits::construct(b.alloc, b.elem + i, std::move(r.elem[i]));
				alloc_traits::destroy(r.alloc, r.elem + i);
			}

			b.sz = r.sz;
			swap(r, b);

			// old r values freed with the destruction of b, no memory leaks
		}

		void resize(size_type newSize, value_type def = value_type{})
		{
			reserve(newSize);
			if (size() < newSize) {

				//std::uninitialized_fill(end(), end() + (newSize - size()), def);
				for (size_type i = size(); i < newSize; ++i)
					alloc_traits::construct(r.alloc, r.elem + i, def);
			}
				
			else if (newSize < size())
			{
				//std::destroy(begin() + newSize, end());
				for (size_type i = newSize; i < size(); ++i)
					alloc_traits::destroy(r.alloc, r.elem + i);
			}
				
			r.sz = newSize;
		}

		void push_back(const value_type& newElem)
		{
			if (capacity() == 0)
			{
				reserve(8);  // default initial space
			}
			else if (size() == capacity())
			{
				reserve(2 * capacity());
			}
			//std::construct_at(end(), newElem);
			alloc_traits::construct(r.alloc, r.elem + r.sz, newElem);
			++r.sz;
		}

		/*
		Typically, people don’t provide list operations, such as insert() and erase(),
		for data types that keep their elements in contiguous storage, such as vector.
		However, list operations, such as insert() and erase(), are immensely useful and
		surprisingly efficient for vectors with small elements or small numbers of
		elements. We have repeatedly seen the usefulness of push_back(), which is
		another operation traditionally associated with lists.
		*/
		iterator insert(iterator p, const value_type& val)
		{
			// TODO: use alloc traits

			size_type index = p - begin(); // save index in case of relocation, iterator invalidation

			if (size() == capacity())
				reserve(size() == 0 ? 8 : 2 * size());

			p = begin() + index;
			std::move_backward(p, end() - 1, p + 1);
			*p = val;
			++r.sz;
			return p;
		}

		iterator erase(iterator p)
		{
			// TODO: Use alloc traits
			if (p == end())
				return p;

			// move elements to one position to the left
			std::move(p + 1, end(), p);
			// destroy surplus last element
			std::destroy_at(end() - 1);
			--r.sz;
			return p;
		}

		// iterator support
		iterator begin() { return r.elem; }
		const_iterator begin() const { return r.elem; }
		iterator end() { return r.elem + r.sz; }
		const_iterator end() const { return r.elem + r.sz; }

		vector_rep<T, A>& data() { return r; }
		const vector_rep<T, A>& data() const { return r; }
	};

	// operators overloads (binary)
	template <typename T>
	std::ostream& operator<<(std::ostream& os, const vector<T>& v)
	{
		os << "size: " << v.size() << "\ncapacity: " << v.capacity() << "\nelem: ";
		for (int i = 0; i < v.capacity(); ++i)
		{
			os << v[i] << ", ";
		}
		return os << "\n";
	}

	template <typename T>
	bool operator==(const vector_rep<T>& a, const vector_rep<T>& b)
	{
		return a.sz == b.sz && std::equal(a.elem, a.elem + a.sz, b.elem);
	}

	template <typename T>
	bool operator!=(const vector_rep<T>& a, const vector_rep<T>& b)
	{
		return !(a == b);
	}

	template <typename T>
	bool operator==(const vector<T>& a, const vector<T>& b)
	{
		return a.data() == b.data();
	}

	template <typename T>
	bool operator!=(const vector<T>& a, const vector<T>& b)
	{
		return !(a == b);
	}

	template<typename T, typename A>
	void swap(vector<T, A>& a, vector<T, A>& b) noexcept {
		std::swap(a.data().alloc, b.data().alloc);
		std::swap(a.data().sz, b.data().sz);
		std::swap(a.data().elem, b.data().elem);
		std::swap(a.data().space, b.data().space);
	}

	template<typename T, typename A>
	void swap(vector_rep<T, A>& a, vector_rep<T, A>& b) noexcept {
		std::swap(a.alloc, b.alloc);
		std::swap(a.sz, b.sz);
		std::swap(a.elem, b.elem);
		std::swap(a.space, b.space);
	}

	template<typename T, typename A>
	void strong_assign(vector<T, A>& target, vector<T, A> arg) // arg passed by value
	{
		std::swap(target, arg);
	}
}



#endif // !MSTL_VECTOR_H