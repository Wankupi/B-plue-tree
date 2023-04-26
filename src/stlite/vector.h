#include <climits>
#include <cstddef>
#include <memory>
#include <vector>
#ifdef USE_STL
namespace kupi {
using std::vector;
}
#else
namespace kupi {

template<typename T, typename Alloc = std::allocator<T>>
class vector {
private:
	class iterator_base_cmp {
	public:
		friend class vector;
		iterator_base_cmp(T *cur) : cur(cur) {}
		bool operator==(const iterator_base_cmp &rhs) const { return cur == rhs.cur; }
		bool operator!=(const iterator_base_cmp &rhs) const { return cur != rhs.cur; }

	protected:
		T *cur;
	};

	template<bool is_const, bool is_reverse>
	class iterator_common : public iterator_base_cmp {
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = typename std::conditional<is_const, const T *, T *>::type;
		using reference = typename std::conditional<is_const, const T &, T &>::type;
		using iterator_category = std::output_iterator_tag;

		using iterator_base_cmp::iterator_base_cmp;

		iterator_common operator+(const int &n) const {
			iterator_common ret = *this;
			return ret += n;
		}
		iterator_common operator-(const int &n) const {
			iterator_common ret = *this;
			return ret -= n;
		}

		difference_type operator-(const iterator_common &rhs) const {
			if constexpr (!is_reverse)
				return this->cur - rhs.cur;
			else
				return rhs.cur - this->cur;
		}

		iterator_common &operator+=(const int &n) {
			if constexpr (!is_reverse)
				this->cur += n;
			else
				this->cur -= n;
			return *this;
		}

		iterator_common &operator-=(const int &n) {
			return *this += -n;
		}

		iterator_common operator++(int) {
			iterator_common ret = *this;
			++*this;
			return ret;
		}
		iterator_common &operator++() {
			if constexpr (!is_reverse)
				++this->cur;
			else
				--this->cur;
			return *this;
		}
		iterator_common operator--(int) {
			iterator_common ret = *this;
			--*this;
			return ret;
		}
		iterator_common &operator--() {
			if constexpr (!is_reverse)
				--this->cur;
			else
				++this->cur;
			return *this;
		}
		reference operator*() const {
			return *this->cur;
		}
		pointer operator->() const { return this->cur; }
	};

public:
	using iterator = iterator_common<false, false>;
	using const_iterator = iterator_common<true, false>;
	using reverse_iterator = iterator_common<false, true>;
	using const_reverse_iterator = iterator_common<true, true>;

	vector() : start{nullptr}, finish{nullptr}, bound{nullptr}, alloc{} {}
	vector(const vector &other) : vector() {
		if (other.empty()) return;
		cover_from_other(other);
	}
	vector(vector &&other) noexcept
		: start{other.start}, finish{other.finish}, bound{other.bound}, alloc{std::move(other.alloc)} {
		other.start = other.finish = other.bound = nullptr;
	}

	~vector() { clear(); }

	vector &operator=(const vector &other) {
		if (this == &other) return *this;
		clear();
		cover_from_other(other);
		return *this;
	}

	vector &operator=(vector &&other) {
		if (this == &other) return *this;
		clear();
		start = other.start;
		finish = other.finish;
		bound = other.bound;
		alloc = std::move(other.alloc);
		other.start = other.finish = other.bound = nullptr;
		return *this;
	}

	T &at(const size_t &pos) {
		return const_cast<T &>(const_cast<const vector<T> *>(this)->at(pos));
	}

	const T &at(const size_t &pos) const {
		return start[pos];
	}
	T &operator[](const size_t &pos) { return at(pos); }
	const T &operator[](const size_t &pos) const { return at(pos); }

	const T &front() const {
		return *start;
	}

	const T &back() const {
		return *(finish - 1);
	}

	iterator begin() { return {start}; }
	const_iterator begin() const { return {start}; }
	const_iterator cbegin() const { return {start}; }
	reverse_iterator rbegin() { return {finish - 1}; }
	const_reverse_iterator rbegin() const { return {finish - 1}; }
	const_reverse_iterator crbegin() const { return {finish - 1}; }

	iterator end() { return {finish}; }
	const_iterator end() const { return {finish}; }
	const_iterator cend() const { return {finish}; };
	reverse_iterator rend() { return {start - 1}; }
	const_reverse_iterator rend() const { return {start - 1}; }
	const_reverse_iterator crend() const { return {start - 1}; }

	[[nodiscard]] bool empty() const { return start == finish; }
	[[nodiscard]] size_t size() const { return finish - start; }
	void clear() {
		size_t sz = finish - start;
		while (finish != start) {
			--finish;
			finish->~T();
		}
		if (start) alloc.deallocate(start, sz);
		start = finish = bound = nullptr;
	}
	iterator insert(iterator pos, const T &value) {
		if (finish == bound) {
			T *dest = register_new_space();
			T *mid = dest + (pos.cur - start);
			copy_or_move(start, pos.cur, dest);
			copy_or_move(pos.cur, finish, mid + 1);
			new (mid) T{value};
			alloc.deallocate(start, finish - start);
			finish = dest + (finish - start) + 1;
			start = dest;
			return {start, mid};
		}
		if (pos.cur < finish) new (finish) T{std::move(*(finish - 1))};
		for (T *p = finish - 1; pos.cur < p; --p) {
			*p = std::move(*(p - 1));
		}
		pos.cur->~T();
		new (pos.cur) T(value);
		++finish;
		return pos;
	}

	iterator insert(const size_t &ind, const T &value) {
		return insert(begin() + ind, value);
	}

	iterator erase(iterator pos) {
		T *cur = pos.cur;
		cur->~T();
		++cur;
		if (cur != finish) {
			new (cur - 1) T(std::move(*cur));
			++cur;
		}
		while (cur != finish) {
			*(cur - 1) = std::move(*cur);
			++cur;
		}
		--finish;
		finish->~T();
		return pos;
	}

	iterator erase(const size_t &ind) {
		return erase(begin() + ind);
	}

	void push_back(const T &value) {
		if (finish == bound) {
			T *dest = register_new_space();
			copy_or_move(start, finish, dest);
			alloc.deallocate(start, finish - start);
			finish = dest + (finish - start);
			start = dest;
		}
		new (finish) T{value};
		++finish;
	}

	template<typename... Argv>
	void emplace_back(Argv &&...args) {
		if (finish == bound) {
			T *dest = register_new_space();
			copy_or_move(start, finish, dest);
			alloc.deallocate(start, finish - start);
			finish = dest + (finish - start);
			start = dest;
		}
		new (finish) T{std::forward<Argv>(args)...};
		++finish;
	}

	void pop_back() {
		--finish;
		finish->~T();
	}

	operator std::vector<T>() const {
		return std::vector<T>{start, finish};
	}

private:
	[[no_unique_address]] Alloc alloc;
	T *start, *finish, *bound;

private:
	void cover_from_other(const vector<T> &other) {
		auto sz = other.size();
		start = alloc.allocate(sz);
		bound = finish = start + sz;
		for (int i = 0; i < sz; ++i)
			new (start + i) T{other.start[i]};
	}

	T *register_new_space() {
		int sz = bound - start;
		if (!sz) sz = 1;
		sz *= 2;
		T *ret = alloc.allocate(sz);
		bound = ret + sz;
		return ret;
	}

	static void copy_or_move(T *src, T *ed, T *dest) {
		while (src != ed) {
			new (dest) T{std::move_if_noexcept(*src)};
			src->~T();
			++src;
			++dest;
		}
	}
};

}// namespace kupi
#endif
