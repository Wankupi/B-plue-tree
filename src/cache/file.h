#pragma once
#include "file/DataBase.h"
namespace kupi {

template<typename T>
class CachePointer {
public:
	CachePointer(nullptr_t) : ptr(nullptr) {}
	CachePointer(T const &data, int id, DataBase<T, true> *db) : ptr(new Node(data, id, db)) {}
	CachePointer(CachePointer const &rhs) : ptr(rhs.ptr) {
		if (ptr) ++(ptr->cnt);
	}
	CachePointer(CachePointer &&rhs) : ptr(rhs.ptr) { rhs.ptr = nullptr; }
	~CachePointer() {
		if (!ptr) return;
		if (--(ptr->cnt)) {
			ptr = nullptr;
			return;
		}
		// if (ptr->stat == Node::Status::write)
		ptr->db->write(ptr->id, ptr->data);
		// else if (ptr->stat == Node::Status::erase)
		// ptr->db->erase(ptr->id);
		delete ptr;
		ptr = nullptr;
	}
	CachePointer &operator=(CachePointer const &rhs) {
		if (this == &rhs) return *this;
		this->~CachePointer();
		new (this) CachePointer(rhs);
		return *this;
	}
	CachePointer &operator=(CachePointer &&rhs) {
		if (this == &rhs) return *this;
		this->~CachePointer();
		new (this) CachePointer(std::move(rhs));
		return *this;
	}
	T *data() { return &ptr->data; }
	T &operator*() { return ptr->data; }
	T *operator->() { return &ptr->data; }
	T const &operator*() const { return ptr->data; }
	T const *operator->() const { return &ptr->data; }
	explicit operator bool() const { return ptr; }

private:
	struct Node {
		Node(T const &data, int id, DataBase<T, true> *db)
			: data(data), cnt(1), id(id), db(db), stat(read) {}
		T data;
		int cnt;
		int id;
		DataBase<T, true> *db;
		enum Status { read,
					  write,
					  erase } stat;
	};
	Node *ptr;
};

template<typename T>
class FileCache {
public:
	FileCache(std::string const &filename) : db(filename) {}
	bool empty() { return db.size() == 0; }
	CachePointer<T> operator[](int id) {
		return {db.read(id), id, &db};
	}
	void deallocate(int id) {
		db.erase(id);
	}
	std::pair<int, CachePointer<T>> allocate() {
		int id = db.insert({});
		return {id, {{}, id, &db}};
	}

private:
	DataBase<T, true> db;
};

}// namespace kupi