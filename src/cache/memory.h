#pragma once
#include <memory>
#include <stack>
#include <unordered_map>

namespace kupi {

template<typename T, typename Alloc = std::allocator<T>>
class MemoryCache {
public:
	MemoryCache() = default;
	MemoryCache(MemoryCache const &rhs) = delete;
	MemoryCache(MemoryCache &&rhs) noexcept = default;
	MemoryCache &operator=(MemoryCache const &rhs) = delete;
	MemoryCache &operator=(MemoryCache &&rhs) noexcept = default;
	~MemoryCache() {
		for (auto [id, p]: data) alloc.deallocate(p, 1);
	}

	bool empty() { return data.size() <= trash.size(); }
	T *operator[](int id) { return data[id]; }

	void deallocate(int id) {
		trash.push(id);
	}
	std::pair<int, T *> allocate() {
		if (!trash.empty()) {
			int id = trash.top();
			trash.pop();
			return {id, data[id]};
		}
		int id = data.size() + 1;
		T *p = alloc.allocate(1);
		data.insert({id, p});
		return {id, p};
	}

private:
	std::unordered_map<int, T *> data;
	std::stack<int> trash;
	Alloc alloc;
};

}// namespace kupi
