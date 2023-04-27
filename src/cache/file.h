#pragma once
#include "file/DataBase.h"
#include "stlite/hash_table.h"
#include "stlite/list.h"
#include "stlite/vector.h"

namespace kupi {

template<typename T>
class FileCache {
	static constexpr int MAX_SIZE = 50;

public:
	FileCache(std::string const &filename) : db(filename) {
		db_size = db.size();
	}
	~FileCache() {
		while (!que.empty())
			delete pop();
		for (int i: deletedIds)
			db.erase(i);
	}
	bool empty() { return db_size - deletedIds.size() == 0; }

	T *operator[](int id) {
		Node n;
		auto p = table.find(id);
		if (p != table.end()) {
			n = *p->second;
			que.erase(p->second);
		}
		else if (que.size() == MAX_SIZE)
			n = load(id, pop());
		else {
			T *t = new T;
			n = load(id, t);
		}
		que.push_back(n);
		table[id] = --que.end();
		return n.data;
	}

	void deallocate(int id) {
		deletedIds.push_back(id);
	}
	std::pair<int, T *> allocate() {
		int id;
		if (!deletedIds.empty()) {
			id = deletedIds.back();
			deletedIds.pop_back();
		}
		else {
			id = db.insert({});
			++db_size;
		}
		return {id, this->operator[](id)};
	}

private:
	struct Node {
		int id;
		T *data;
	};
	T *pop() {
		Node nd = que.front();
		que.pop_front();
		table.erase(nd.id);
		db.write(nd.id, *nd.data);
		return nd.data;
	}
	Node load(int id, T *res) {
		db.read(id, *res);
		return {id, res};
	}

private:
	DataBase<T, true> db;
	list<Node> que;
	unordered_map<int, typename list<Node>::iterator> table;
	vector<int> deletedIds;
	int db_size; // to reduce time on db.size()
};

}// namespace kupi
