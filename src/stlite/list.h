#pragma once
#ifdef USE_STL
#include <list>
namespace kupi {
using std::list;
}
#else
namespace kupi {

template<typename T>
class list {
	struct Node {
		Node *last = nullptr, *next = nullptr;
		T data{};
	};

public:
	struct iterator {
		Node *it;
		T &operator*() const { return it->data; }
		T *operator->() const { return &it->data; }
		iterator &operator--() {
			it = it->last;
			return *this;
		}
	};

public:
	list() : head{new Node{}}, tail{new Node{}}, siz(0) {
		head->next = tail;
		tail->last = head;
	}
	~list() {
		while (head) {
			Node *t = head;
			head = head->next;
			delete t;
		}
	}
	[[nodiscard]] bool empty() const { return !siz; }
	[[nodiscard]] size_t size() const { return siz; }
	void erase(iterator it) { erase(it.it); }
	iterator end() const { return {tail}; }
	T &front() { return head->next->data; }
	void pop_front() { erase(head->next); }
	void push_back(T const &t) {
		Node *nd = new Node{tail->last, tail, t};
		tail->last->next = nd;
		tail->last = nd;
		++siz;
	}

private:
	void erase(Node *p) {
		p->last->next = p->next;
		p->next->last = p->last;
		delete p;
		--siz;
	}

private:
	Node *head = nullptr, *tail = nullptr;
	size_t siz;
};

}// namespace kupi
#endif