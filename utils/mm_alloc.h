template <typename T, std::size_t Alignment = 16>
class _mm_allocator {
	enum { alignment = Alignment };
public:
	typedef T * pointer;
	typedef const T * const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;
	typedef std::size_t size_type;
	typedef ptrdiff_t difference_type;

	T * address(T& r) const { return &r; }
	const T * address(const T& s) const { return &s; }
	std::size_t max_size() const { return (static_cast<std::size_t>(0) - static_cast<std::size_t>(1)) / sizeof(T); }

	template <typename U> struct rebind { typedef _mm_allocator<U, Alignment> other; };

	bool operator==(const _mm_allocator& other) const { return true; }
	bool operator!=(const _mm_allocator& other) const { return !(*this == other); }

	void construct(T * const p, const T& t) const {
		void* const pv = static_cast<void *>(p);
		new (pv) T(t);
	}

	void destroy(T * const p) const {
		p->~T();
	}

	_mm_allocator() { }
	_mm_allocator(const _mm_allocator&) { }
	template <typename U> _mm_allocator(const _mm_allocator<U, Alignment>&) { }
	~_mm_allocator() { }


	T* allocate(const std::size_t n) const {
		if (n == 0) return NULL;
		if (n > max_size()) throw std::length_error("_mm_allocator<T>::allocate() - Integer overflow.");
		void * const pv = _mm_malloc(n * sizeof(T), Alignment);
		if (pv == NULL) {
			int err = errno;
			char* info = strerror(err);
			throw std::bad_alloc();
		}
		return static_cast<T *>(pv);
	}

	void deallocate(T * const p, const std::size_t n) const	{
		_mm_free(p);
	}

	template <typename U>
	T * allocate(const std::size_t n, const U * /* const hint */) const {
		return allocate(n);
	}

private:
	_mm_allocator& operator=(const _mm_allocator&);
};
