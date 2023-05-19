#pragma once
#include "cache/file.h"
#include "cache/memory.h"
#include "stlite/algorithm.h"
#include <cstring>
#include <memory>

namespace kupi {

template<typename Key, typename Val, template<typename> class Array = FileCache>
class bpt {
private:
	using pair = std::pair<Key, Val>;
	constexpr static int BLOCK_SIZE = 4096;
	struct node_meta {
		int size;
		int last_child;
		bool is_leaf;
	};
	struct node_data {
		Key key;
		int child;
	};
	struct node {
		constexpr static int M = (BLOCK_SIZE - sizeof(node_meta)) / sizeof(node_data);
		node_meta header;
		node_data data[M];
		node_data *end() { return data + header.size; }
	};
	struct leaf_meta {
		int size;
		int next;
	};
	struct leaf {
		pair *end() { return data + header.size; }
		pair const &back() const { return data[header.size - 1]; }
		// at least M=3
		constexpr static int M = (BLOCK_SIZE - sizeof(leaf_meta)) / sizeof(pair);
		leaf_meta header;
		pair data[M];
	};

public:
	class iterator {
		friend class bpt;
		iterator(pair *_ptr, leaf *_leaf, Array<leaf> *ar) : ptr{_ptr}, p_leaf{_leaf}, leave{ar} {}

	public:
		using value_type = std::pair<Key const, Val>;
		using pointer = value_type *;
		using reference = value_type &;
		reference operator*() { return *reinterpret_cast<pointer>(ptr); }
		pointer operator->() { return reinterpret_cast<pointer>(ptr); }
		iterator &operator++() {
			if (!ptr) return *this;
			if (ptr != &p_leaf->back())
				++ptr;
			else {
				if (p_leaf->header.next) {
					p_leaf = (*leave)[p_leaf->header.next];
					ptr = p_leaf->data;
				}
				else {
					p_leaf = nullptr;
					ptr = nullptr;
				}
			}
			return *this;
		}
		bool operator==(iterator const &rhs) const { return ptr == rhs.ptr; }
		bool operator!=(iterator const &rhs) const { return ptr != rhs.ptr; }

	private:
		pair *ptr;
		leaf *p_leaf;
		Array<leaf> *leave;
	};

public:
	bpt() = default;
	explicit bpt(std::string const &tr_name) : nodes(tr_name + ".nodes"), leave(tr_name + ".leave") {}
	void insert(Key const &key, Val const &val) { insert({key, val}); }
	void insert(std::pair<Key, Val> const &x);
	void erase(Key const &key);
	iterator find(Key const &key);
	iterator end() { return {nullptr, nullptr, &leave}; }

private:
	// root is fixed as nodes[1], or leave[1]
	Array<node> nodes;
	Array<leaf> leave;

private:
	using node_ptr = decltype(nodes[1]);
	using leaf_ptr = decltype(leave[1]);
	static bool cmp_key_node(node_data const &A, node_data const &B) { return A.key < B.key; }
	static bool cmp_key_leaf(pair const &A, pair const &B) { return A.first < B.first; }
	/**
	 * @param index key
	 * @param st the nodes will be stored in it
	 * @return pointer to the leaf where x should lay in. store the chain to it in a vector
	 */
	leaf_ptr find_leaf(Key const &key, vector<std::pair<node_ptr, node_data *>> &st);

	/**
	 * The following 3 methods are called in insert.
	 * new_node == 0 means nothing should be inserted into the father of this node.
	 * otherwise it's the id of the new node(leaf).
	 */
	struct insert_result {
		int new_node = 0;
		Key const *spilt_key = nullptr;
	};
	insert_result insert_leaf(leaf_ptr p, pair const &x);
	insert_result insert_node(node_ptr p, node_data *k, insert_result const &ir);
	void insert_new_root(insert_result const &ir);

	/**
	 * after a erase action on a node,
	 * its back value might change
	 * its length might be unsatisfied with the definition of B+tree
	 */
	struct erase_result {
		Key const *update_key = nullptr;
		bool is_short = false;
	};
	erase_result erase_leaf(leaf_ptr p, Key const &key);
	bool erase_node(node_ptr p, node_data *k);
	/**
	 * reassign* make p and q more balance.
	 * if they could be put in a single node or leaf, then merge their data to p
	 * otherwise average p and q.
	 */
	enum class reassign_method { average,
								 merge };
	reassign_method reassign_leaf(leaf_ptr p, Key &key, leaf_ptr q);
	reassign_method reassign_node(node_ptr p, Key &key, node_ptr q);
	reassign_method reassign(int p, Key &key, int q, bool is_leaf) { return is_leaf ? reassign_leaf(leave[p], key, leave[q]) : reassign_node(nodes[p], key, nodes[q]); }
};

template<typename Key, typename Val, template<typename> class Array>
bpt<Key, Val, Array>::iterator bpt<Key, Val, Array>::find(Key const &key) {
	if (leave.empty()) return end();
	auto p = nodes.empty() ? nullptr : nodes[1];
	auto ptr = nodes.empty() ? leave[1] : nullptr;
	while (p) {
		// cast to longer is ok because cmp_key_node will only access to `node_data::key`, i.e. `key`.
		auto *k = lower_bound(p->data, p->end(), *reinterpret_cast<node_data const*>(&key), cmp_key_node);
		auto next = k == p->end() ? p->header.last_child : k->child;
		if (p->header.is_leaf) {
			ptr = leave[next];
			break;
		}
		p = nodes[next];
	}
	auto k = lower_bound(ptr->data, ptr->end(), pair{key, {}}, cmp_key_leaf);
	if (k == ptr->end() || k->first != key)
		return end();
	return {k, ptr, &leave};
}

template<typename Key, typename Val, template<typename> class Array>
typename bpt<Key, Val, Array>::leaf_ptr bpt<Key, Val, Array>::find_leaf(Key const &key, vector<std::pair<node_ptr, node_data *>> &st) {
	if (nodes.empty())
		return leave[1];// tree is not empty
	node_ptr p = nodes[1];
	while (true) {
		// cast to longer is ok because cmp_key_node will only access to `node_data::key`, i.e. `key`.
		auto k = lower_bound(p->data, p->end(), *reinterpret_cast<node_data const*>(&key), cmp_key_node);
		st.emplace_back(p, k);
		auto next = k == p->end() ? p->header.last_child : k->child;
		if (p->header.is_leaf)
			return leave[next];
		else
			p = nodes[next];
	}
}

template<typename Key, typename Val, template<typename> class Array>
void bpt<Key, Val, Array>::insert(std::pair<Key, Val> const &x) {
	if (leave.empty()) {
		auto [id, p] = leave.allocate();// the first id is 1
		p->header = leaf_meta{1, 0};
		p->data[0] = x;
		return;
	}
	vector<std::pair<node_ptr, node_data *>> st;
	auto ptr = find_leaf(x.first, st);
	auto ir = insert_leaf(ptr, x);
	if (!ir.new_node) return;
	for (auto cur = st.rbegin(); ir.new_node && cur != st.rend(); ++cur)
		ir = insert_node(cur->first, cur->second, ir);
	if (ir.new_node)
		insert_new_root(ir);
}

template<typename Key, typename Val, template<typename> class Array>
typename bpt<Key, Val, Array>::insert_result bpt<Key, Val, Array>::insert_leaf(leaf_ptr p, pair const &x) {
	auto k = lower_bound(p->data, p->end(), x, cmp_key_leaf);
	if (k != p->end() && k->first == x.first) return {0, nullptr};
	pair last = k == p->end() ? x : p->back();// ==x might happen when p is the back leaf on the tree
	if (int count = (p->data + std::min(p->header.size, leaf::M - 1) - k); count > 0)
		memmove(k + 1, k, count * sizeof(pair));
	if (p->header.size < leaf::M || k != p->end())
		*k = x;
	if (++(p->header.size) <= leaf::M) return {0, nullptr};
	constexpr int A = (leaf::M + 2) / 2, B = leaf::M + 1 - A;
	auto [q_id, q] = leave.allocate();
	q->header = leaf_meta{B, p->header.next};
	p->header = leaf_meta{A, q_id};
	memcpy(q->data, p->data + A, (leaf::M - A) * sizeof(pair));
	q->data[B - 1] = last;
	return {q_id, &p->back().first};
}

template<typename Key, typename Val, template<typename> class Array>
typename bpt<Key, Val, Array>::insert_result bpt<Key, Val, Array>::insert_node(node_ptr p, node_data *k, insert_result const &ir) {
	node_data X{*ir.spilt_key, 0};
	node_data last;
	if (k != p->end()) {
		X.child = k->child;
		k->child = ir.new_node;
		last = p->data[p->header.size - 1];
	}
	else {
		X.child = p->header.last_child;
		p->header.last_child = ir.new_node;
		last = X;
	}
	if (int count = (p->header.size == node::M ? p->end() - 1 : p->end()) - k; count > 0)
		memmove(k + 1, k, count * sizeof(node_data));
	if (p->header.size < node::M || k != p->end())
		*k = X;
	if (++(p->header.size) <= node::M) return {0};
	constexpr int A = (node::M + 1) / 2, B = node::M - A;
	auto [id, q] = nodes.allocate();
	q->header = node_meta{B, p->header.last_child, p->header.is_leaf};
	p->header = node_meta{A, p->data[A].child, p->header.is_leaf};
	memcpy(q->data, p->data + A + 1, (node::M - A - 1) * sizeof(node_data));
	q->data[B - 1] = last;
	// return reference is not very safe. require cache support
	return {id, &p->data[A].key};
}

template<typename Key, typename Val, template<typename> class Array>
void bpt<Key, Val, Array>::insert_new_root(insert_result const &ir) {
	bool is_leaf = nodes.empty();
	auto [id, q] = nodes.allocate();
	if (is_leaf) {
		q->header = node_meta{1, ir.new_node, true};
		q->data[0] = {*ir.spilt_key, 1};
	}
	else {
		auto rt = nodes[1];
		*q = *rt;
		rt->header = node_meta{1, ir.new_node, false};
		rt->data[0] = {*ir.spilt_key, id};
	}
}

template<typename Key, typename Val, template<typename> class Array>
void bpt<Key, Val, Array>::erase(const Key &key) {
	if (leave.empty()) return;
	vector<std::pair<node_ptr, node_data *>> st;
	auto ptr = find_leaf(key, st);
	auto er = erase_leaf(ptr, key);
	// deal with the change of key first
	if (er.update_key) {
		for (auto cur = st.rbegin(); cur != st.rend(); ++cur)
			if (cur->second != cur->first->data + cur->first->header.size) {
				cur->second->key = *er.update_key;
				break;
			}
	}
	for (auto cur = st.rbegin(); er.is_short && cur != st.rend(); ++cur)
		er.is_short = erase_node(cur->first, cur->second);
	if (er.is_short) {
		if (nodes.empty()) return;
		node_ptr rt = st[0].first;
		if (rt->header.size) return;
		if (rt->header.is_leaf) {
			nodes.deallocate(1);
			return;
		}
		int old = rt->header.last_child;
		auto q = nodes[old];
		*rt = *q;
		nodes.deallocate(old);
	}
}

template<typename Key, typename Val, template<typename> class Array>
typename bpt<Key, Val, Array>::erase_result bpt<Key, Val, Array>::erase_leaf(leaf_ptr p, Key const &key) {
	pair *k = lower_bound(p->data, p->end(), pair{key, {}}, cmp_key_leaf);
	if (k == p->end() || k->first != key) return {};
	memmove(k, k + 1, (p->end() - k - 1) * sizeof(pair));
	--(p->header.size);
	Key const *back = (k == p->end() ? &p->back().first : nullptr);
	return {back, p->header.size < (leaf::M + 1) / 2};
}

template<typename Key, typename Val, template<typename> class Array>
bool bpt<Key, Val, Array>::erase_node(node_ptr p, node_data *k) {
	auto get_size = [this, is_leaf = p->header.is_leaf](int id) {
		if (!id) return 0;
		return is_leaf ? leave[id]->header.size : nodes[id]->header.size;
	};
	auto pre = k == p->data ? 0 : (k - 1)->child;
	auto next = (k == p->end() ? 0 : (k == p->end() - 1 ? p->header.last_child : (k + 1)->child));
	int siz_pre = get_size(pre), siz_nxt = get_size(next);
	node_data *to_erase;
	if (siz_pre > siz_nxt) {
		auto rr = reassign(pre, (k - 1)->key, k < p->end() ? k->child : p->header.last_child, p->header.is_leaf);
		if (rr == reassign_method::average)
			return false;
		to_erase = k;
	}
	else {
		auto rr = reassign(k->child, k->key, k == p->end() - 1 ? p->header.last_child : (k + 1)->child, p->header.is_leaf);
		if (rr == reassign_method::average)
			return false;
		to_erase = k + 1;
	}
	int to_release;
	if (to_erase < p->end()) {
		to_release = to_erase->child;
		(to_erase - 1)->key = to_erase->key;
		memmove(to_erase, to_erase + 1, (p->end() - to_erase - 1) * sizeof(node_data));
		--(p->header.size);
	}
	else {
		to_release = p->header.last_child;
		--(p->header.size);
		p->header.last_child = p->end()->child;
	}
	if (p->header.is_leaf) leave.deallocate(to_release);
	else
		nodes.deallocate(to_release);
	return (p->header.size + 1) < (node::M + 2) / 2;
}

template<typename Key, typename Val, template<typename> class Array>
typename bpt<Key, Val, Array>::reassign_method bpt<Key, Val, Array>::reassign_leaf(leaf_ptr p, Key &key, leaf_ptr q) {
	if (p->header.size + q->header.size > leaf::M) {
		// average
		const int A = (p->header.size + q->header.size + 1) / 2, B = p->header.size + q->header.size - A;
		// assert(A != p->header.size);
		// assert(B != q->header.size);
		if (A - p->header.size > 0) memcpy(p->data + p->header.size, q->data, (A - p->header.size) * sizeof(pair));
		if (q->header.size < B) {
			memmove(q->data + B - q->header.size, q->data, q->header.size * sizeof(pair));
			memcpy(q->data, p->data + A, (B - q->header.size) * sizeof(pair));
		}
		else
			memmove(q->data, q->data + q->header.size - B, B * sizeof(pair));
		p->header.size = A;
		q->header.size = B;
		key = p->back().first;
		return reassign_method::average;
	}
	else {
		// merge
		memcpy(p->data + p->header.size, q->data, q->header.size * sizeof(pair));
		p->header.size += q->header.size;
		p->header.next = q->header.next;
		return reassign_method::merge;
	}
}

template<typename Key, typename Val, template<typename> class Array>
typename bpt<Key, Val, Array>::reassign_method bpt<Key, Val, Array>::reassign_node(node_ptr p, Key &key, node_ptr q) {
	if (p->header.size + q->header.size >= node::M) {
		// average
		int A = (p->header.size + q->header.size + 1) / 2, B = p->header.size + q->header.size - A;
		if (A > p->header.size) {
			p->data[p->header.size] = {key, p->header.last_child};
			memcpy(p->data + p->header.size + 1, q->data, (A - p->header.size - 1) * sizeof(node_data));
			p->header.last_child = q->data[q->header.size - B - 1].child;
			key = q->data[q->header.size - B - 1].key;
			memmove(q->data, q->data + q->header.size - B, B * sizeof(node_data));
		}
		else {
			memmove(q->data + B - q->header.size, q->data, (q->header.size) * sizeof(node_data));
			memcpy(q->data, p->data + A + 1, (p->header.size - A - 1) * sizeof(node_data));
			q->data[p->header.size - A - 1] = {key, p->header.last_child};
			p->header.last_child = p->data[A].child;
			key = p->data[A].key;
		}
		p->header.size = A;
		q->header.size = B;
		return reassign_method::average;
	}
	else {
		// merge
		p->data[p->header.size] = {key, p->header.last_child};
		memcpy(p->data + p->header.size + 1, q->data, q->header.size * sizeof(node_data));
		p->header.size += q->header.size + 1;
		p->header.last_child = q->header.last_child;
		return reassign_method::merge;
	}
}

}// namespace kupi
