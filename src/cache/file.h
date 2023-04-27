#pragma once
#include "file/DataBase.h"
#include "stlite/hash_table.h"
#include "stlite/list.h"
#include "stlite/vector.h"
#include <cstring>

namespace kupi {

template<typename T>
class FileCache {
	static constexpr int MAX_SIZE = 50;

public:
	FileCache(std::string const &filename) : db(filename) {
		db_size = db.size();
	}
	~FileCache() {
		for (Node const &nd : que) {
			flush_out(nd);
			delete[] nd.data;
		}
		for (int i: deletedIds)
			db.erase(i);
	}
	bool empty() { return db_size - deletedIds.size() == 0; }

	T *operator[](int id) {
		if (auto p = table.find(id); p != table.end()) {
			que.splice(que.end(), que, p->second);
			return p->second->data;
		}
		if (que.size() == MAX_SIZE) {
			auto p = que.begin();
			que.splice(que.end(), que, p);
			table.erase(p->id);
			flush_out(*p);
			p->id = id;
			load(id, p->data);
			table[id] = p;
			return p->data;
		}
		else {
			T* dest = new T[2];
			que.push_back({id, dest});
			load(id, dest);
			table[id] = --que.end();
			return dest;
		}
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
	void flush_out(Node const &nd) {
		if (memcmp(nd.data, nd.data + 1, sizeof(T)))
			db.write(nd.id, *nd.data);
	}
	void load(int id, T *res) {
		db.read(id, *res);
		memcpy(res + 1, res, sizeof(T));
	}

private:
	DataBase<T, true> db;
	list<Node> que;
	unordered_map<int, typename list<Node>::iterator> table;
	vector<int> deletedIds;
	int db_size;// to reduce time on db.size()
};

}// namespace kupi
