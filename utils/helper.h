#ifndef __helper_h__
#define __helper_h__

//template <typename T> using sptr = std::shared_ptr;

//namespace mx {
	//?template <typename T> using map = std::map<Key, Value, _mm_allocator<>>;
	//template <typename T> using stack = std::stack<T, std::deque<T, _mm_allocator<T>>>;
	//template <typename T> using vector = std::vector<T, _mm_allocator<T>>;
//}

namespace helper {

template <typename Value>
class list_of {
public:
	typedef std::vector<Value> Data;

public:
	list_of() {}

	list_of(const Value& value) {
		m_data.push_back(value);
	}

	list_of& operator () (const Value& value) {
		m_data.push_back(value);
		return *this;
	}

	operator const Data& () const {
		return m_data;
	}

public:
	Data m_data;
};

template <typename Key, typename Value>
class map_list_of {
public:
	typedef std::map<Key, Value> Data;

public:
	map_list_of() {}

	map_list_of(const Key& key, const Value& value) {
		m_data.insert(std::make_pair(key, value));
	}

	map_list_of& operator () (const Key& key, const Value& value) {
		m_data.insert(std::make_pair(key, value));
		return *this;
	}

	operator const Data& () const {
		return m_data;
	}

public:
	Data m_data;
};

}

#endif // __helper_h__
