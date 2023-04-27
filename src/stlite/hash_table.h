#pragma once
#include <utility>// to use std::pair
#ifdef USE_STL
#include <unordered_map>
namespace kupi {
using std::unordered_map;
}
#else
namespace kupi {

constexpr int hash(int x) {
	return (x << 3) ^ (x >> 7);
}

template<typename Key, typename Val, unsigned int LEN = 1 << 16>// LEN is a power of 2
class unordered_map {
	struct Node {
		std::pair<const Key, Val> x;
		Node *next;
	};

public:
	struct iterator {
		std::pair<const Key, Val> &operator*() { return nd->x; }
		std::pair<const Key, Val> *operator->() { return &nd->x; }
		bool operator==(iterator const &rhs) const { return nd == rhs.nd; }
		bool operator!=(iterator const &rhs) const { return nd != rhs.nd; }
		Node *nd = nullptr;
	};
	unordered_map() : array{new Node *[LEN] { nullptr }} {}
	~unordered_map() {
		for (int i = 0; i < LEN; ++i) {
			while (array[i]) {
				Node *t = array[i];
				array[i] = t->next;
				delete t;
			}
		}
		delete[] array;
	}
	iterator end() { return {nullptr}; }
	iterator find(Key const key) const {
		Node *t = array[hash(key) & (LEN - 1)];
		while (t && t->x.first != key) t = t->next;
		return {t};
	}

	void erase(Key const &key) {
		Node **pre = array + (hash(key) & (LEN - 1));
		Node *t = *pre;
		while (t && t->x.first != key) t = *(pre = &t->next);
		if (!t) return;
		*pre = t->next;
		delete t;
	}

	Val &operator[](Key const &key) {
		return insert(key)->x.second;
	}

private:
	Node *insert(Key const &key) {
		Node **head = array + (hash(key) & (LEN - 1));
		Node *t = *head;
		while (t && t->x.first != key) t = t->next;
		if (!t) *head = t = new Node{{key, {}}, *head};
		return t;
	}

private:
	Node **array;
};

}// namespace kupi

#endif
