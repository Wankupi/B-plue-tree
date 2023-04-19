#pragma
#include "bpt_exception.h"
#include "cache/memory.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>

namespace kupi {

using Key = long long;
using Val = int;
template<typename Type>
using Array = MemoryCache<Type>;
//template<typename Key, typename Val, template<typename Type> class Array = MemoryCache>
class bpt {
public:
	// TODO: construct function
	// TODO: deconstruct function
	void insert(Key const &index, Val const &val);
	void erase(Key const &index, Val const &val);
	std::vector<Val> find(Key const &index);

private:
	constexpr static int BLOCK_SIZE = 4096;
	struct pair {
		Key key;
		Val val;
		friend bool operator<(pair const &lhs, pair const &rhs) {
			return lhs.key == rhs.key ? lhs.val < rhs.val : lhs.key < rhs.key;
		}
		bool operator==(pair const &rhs) const = default;
	};
	struct node {
		struct node_meta {
			int size;
			int last_child;
			bool is_leaf;
		};
		struct node_data {
			pair key;
			int child;
		};
		constexpr static int M = (BLOCK_SIZE - sizeof(node_meta)) / sizeof(node_data);
		node_meta header;
		node_data data[M];
		// char padding[BLOCK_SIZE - sizeof(header) - sizeof(data)];
		node_data *end() { return data + header.size; }
	};
	struct leaf {
		struct leaf_meta {
			int size;
			int last, next;
		};
		constexpr static int M = (BLOCK_SIZE - sizeof(leaf_meta)) / sizeof(pair);
		leaf_meta header;
		pair data[M];
		// char padding[BLOCK_SIZE - sizeof(int) - sizeof(data)];
		pair *end() { return data + header.size; }
		pair const &back() const { return data[header.size - 1]; }
	};

private:
	// root is fixed as id 1
	Array<node> nodes;
	Array<leaf> leave;

private:
	std::pair<int, leaf *> find_leaf(pair const &x, std::vector<std::pair<node *, node::node_data *>> &st);

	struct insert_result {
		int new_node = 0;
		pair const *spilt_key = nullptr;
	};
	insert_result insert_leaf(int id, leaf *p, pair const &x);
	insert_result insert_node(node *p, node::node_data *k, insert_result const &ir);
	void insert_new_root(insert_result const &ir);

	struct erase_result {
		pair const *update_key = nullptr;
		bool is_short = false;
	};
	erase_result erase_leaf(leaf *p, pair const &x);
	bool erase_node(node *p, node::node_data *k);
	enum class reassign_result { average,
								 merge };
	reassign_result reassign_leaf(leaf *p, pair &key, leaf *q, int p_id);
	reassign_result reassign_node(node *p, pair &key, node *q);
	reassign_result reassign(int p, pair &key, int q, bool is_leaf) { return is_leaf ? reassign_leaf(leave[p], key, leave[q], p) : reassign_node(nodes[p], key, nodes[q]); }
};

std::vector<Val> bpt::find(Key const &index) {
	constexpr auto cmp_key_node = [](node::node_data const &A, node::node_data const &B) { return A.key.key < B.key.key; };
	constexpr auto cmp_key_leaf = [](pair const &A, pair const &B) { return A.key < B.key; };
	if (leave.empty()) return {};
	pair x{index, {}};
	node::node_data X{x, 0};
	node *p = nodes.empty() ? nullptr : nodes[0];
	leaf *ptr = nodes.empty() ? leave[1] : nullptr;
	while (p) {
		auto *k = std::lower_bound(p->data, p->data + p->header.size, X, cmp_key_node);
		auto next = k == p->data + p->header.size ? p->header.last_child : k->child;
		if (p->header.is_leaf) {
			ptr = leave[next];
			break;
		}
		p = nodes[next];
	}
	std::vector<Val> res;
	auto k = std::lower_bound(ptr->data, ptr->data + ptr->header.size, x, cmp_key_leaf);
	while (ptr) {
		while (k < ptr->data + ptr->header.size && index == k->key) {
			res.emplace_back(k->val);
			++k;
		}
		if (k != ptr->data + ptr->header.size)
			break;
		ptr = leave[ptr->header.next];
		k = ptr->data;
	}
	return res;
}

std::pair<int, bpt::leaf *> bpt::find_leaf(pair const &x, std::vector<std::pair<node *, node::node_data *>> &st) {
	if (nodes.empty())
		return {1, leave[1]};// tree is not empty
	node::node_data X{x, 0};
	node *p = nodes[1];
	constexpr auto cmp_key_node = [](node::node_data const &A, node::node_data const &B) { return A.key < B.key; };
	while (true) {
		auto k = std::lower_bound(p->data, p->data + p->header.size, X, cmp_key_node);
		st.push_back({p, k});
		auto next = k == p->data + p->header.size ? p->header.last_child : k->child;
		if (p->header.is_leaf)
			return {next, leave[next]};
		else
			p = nodes[next];
	}
}

// 1. find leaf
// 2. add
// 3. adjust up
void bpt::insert(Key const &index, Val const &val) {
	pair x{index, val};
	if (leave.empty()) {
		auto [id, p] = leave.allocate();// it's id must be 1, ensured by Alloc
		// memset(p, 0, sizeof(leaf));
		p->header = leaf::leaf_meta{1, 0, 0};
		p->data[0] = x;
		return;
	}
	std::vector<std::pair<node *, node::node_data *>> st;
	auto [id, ptr] = find_leaf(x, st);
	auto ir = insert_leaf(id, ptr, x);
	if (!ir.new_node) return;
	for (auto cur = st.rbegin(); ir.new_node && cur != st.rend(); ++cur)
		ir = insert_node(cur->first, cur->second, ir);
	if (ir.new_node)
		insert_new_root(ir);
}

bpt::insert_result bpt::insert_leaf(int p_id, leaf *p, pair const &x) {
	auto k = std::lower_bound(p->data, p->end(), x);
	if (k != p->end() && *k == x) return {0, nullptr};
	pair last = k == p->end() ? x : p->back();// ==x might happen when p is the back leaf on the tree
	for (auto cur = p->data + std::min(p->header.size, leaf::M - 1); cur > k; --cur)
		*cur = *(cur - 1);
	if (p->header.size < leaf::M || k != p->end())
		*k = x;
	if (++(p->header.size) <= leaf::M) return {0, nullptr};
	constexpr int A = (leaf::M + 2) / 2, B = leaf::M + 1 - A;
	// leaf *q = new leaf;
	auto [q_id, q] = leave.allocate();
	q->header = leaf::leaf_meta{B, p_id, p->header.next};
	p->header = leaf::leaf_meta{A, p->header.last, q_id};
	for (int i = A; i < leaf::M; ++i)
		q->data[i - A] = p->data[i];
	q->data[B - 1] = last;
	return {q_id, &p->back()};
}

bpt::insert_result bpt::insert_node(node *p, node::node_data *k, insert_result const &ir) {
	node::node_data X{*ir.spilt_key, 0};
	node::node_data last;
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
	for (auto cur = p->header.size == node::M ? p->end() - 1 : p->end(); cur > k; --cur)
		*cur = *(cur - 1);
	if (p->header.size < node::M || k != p->end())
		*k = X;
	if (++(p->header.size) <= node::M) return {0};
	constexpr int A = (node::M + 1) / 2, B = node::M - A;
	auto [id, q] = nodes.allocate();
	q->header = node::node_meta{B, p->header.last_child, p->header.is_leaf};
	p->header = node::node_meta{A, p->data[A].child, p->header.is_leaf};
	for (int i = A + 1; i < node::M; ++i)
		q->data[i - A - 1] = p->data[i];
	q->data[B - 1] = last;
	// return reference is not very safe... just do it in memory for convenience
	return {id, &p->data[A].key};
}

void bpt::insert_new_root(insert_result const &ir) {
	bool is_leaf = nodes.empty();
	auto [id, q] = nodes.allocate();
	if (is_leaf) {
		q->header = node::node_meta{1, ir.new_node, true};
		q->data[0] = {*ir.spilt_key, 1};
	}
	else {
		auto rt = nodes[1];
		memcpy(q, rt, sizeof(node));
		rt->header = node::node_meta{1, ir.new_node, false};
		rt->data[0] = {*ir.spilt_key, id};
	}
}

void bpt::erase(const Key &index, const Val &val) {
	if (leave.empty()) return;
	pair x{index, val};
	std::vector<std::pair<node *, node::node_data *>> st;
	auto [p_id, ptr] = find_leaf(x, st);
	auto er = erase_leaf(ptr, x);
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
		node *rt = st[0].first;
		if (rt->header.size) return;
		int old = rt->header.last_child;
		auto q = nodes[old];
		memcpy(rt, q, sizeof(node));
		nodes.deallocate(old);
	}
}

bpt::erase_result bpt::erase_leaf(leaf *p, pair const &x) {
	auto k = std::lower_bound(p->data, p->end(), x);
	if (k == p->end() || *k != x) return {};
	for (auto cur = k + 1; cur < p->end(); ++cur)
		*(cur - 1) = *cur;
	--(p->header.size);
	pair const *back = (k == p->end() ? &p->back() : nullptr);
	return {back, p->header.size < (leaf::M + 1) / 2};
}

bool bpt::erase_node(node *p, node::node_data *k) {
	auto get_size = [this, is_leaf = p->header.is_leaf](int id) {
		if (!id) return 0;
		return is_leaf ? leave[id]->header.size : nodes[id]->header.size;
	};
	auto pre = k == p->data ? 0 : (k - 1)->child;
	auto next = (k == p->end() ? 0 : (k == p->end() - 1 ? p->header.last_child : (k + 1)->child));
	int siz_pre = get_size(pre), siz_nxt = get_size(next);
	node::node_data *to_erase = nullptr;
	if (siz_pre > siz_nxt) {
		auto rr = reassign(pre, (k - 1)->key, k < p->end() ? k->child : p->header.last_child, p->header.is_leaf);
		if (rr == reassign_result::average)
			return false;
		to_erase = k;
	}
	else {
		auto rr = reassign(k->child, k->key, k == p->end() - 1 ? p->header.last_child : (k + 1)->child, p->header.is_leaf);
		if (rr == reassign_result::average)
			return false;
		to_erase = k + 1;
	}
	int to_release = 0;
	if (to_erase < p->end()) {
		(to_erase - 1)->key = to_erase->key;
		for (auto cur = to_erase + 1; cur < p->end(); ++cur) *(cur - 1) = *cur;
		--(p->header.size);
		to_release = to_erase->child;
	}
	else {
		to_release = p->header.last_child;
		--(p->header.size);
		p->header.last_child = p->end()->child;
	}
	if (p->header.is_leaf) leave.deallocate(to_release);
	else
		nodes.deallocate(to_release);
	return p->header.size < (node::M + 1) / 2;
}

bpt::reassign_result bpt::reassign_leaf(leaf *p, pair &key, leaf *q, int p_id) {
	if (p->header.size + q->header.size > leaf::M) {
		// average
		int A = (p->header.size + q->header.size + 1) / 2, B = p->header.size + q->header.size - A;
		// assert(A != p->header.size);
		// assert(B != q->header.size);
		for (int i = p->header.size; i < A; ++i) p->data[i] = q->data[i - p->header.size];
		if (q->header.size < B) {
			for (int i = 1; i <= q->header.size; ++i) q->data[B - i] = q->data[q->header.size - i];
			for (int i = 0; i < B - q->header.size; ++i) q->data[i] = p->data[A + i];
		}
		else
			for (int i = 0; i < B; ++i) q->data[i] = q->data[q->header.size - B + i];
		p->header.size = A;
		q->header.size = B;
		key = p->back();
		return reassign_result::average;
	}
	else {
		// merge
		for (int i = 0; i < q->header.size; ++i) p->data[p->header.size + i] = q->data[i];
		p->header.size += q->header.size;
		p->header.next = q->header.next;
		if (p->header.next) leave[p->header.next]->header.last = p_id;
		return reassign_result::merge;
	}
}

bpt::reassign_result bpt::reassign_node(node *p, pair &key, node *q) {
	if (p->header.size + q->header.size >= node::M) {
		// average
		int A = (p->header.size + q->header.size + 1) / 2, B = p->header.size + q->header.size - A;
		if (A > p->header.size) {
			p->data[p->header.size] = {key, p->header.last_child};
			for (int i = p->header.size + 1; i < A; ++i) p->data[i] = q->data[i - p->header.size - 1];
			p->header.last_child = q->data[q->header.size - B - 1].child;
			key = q->data[q->header.size - B - 1].key;
			for (int i = 0; i < B; ++i) q->data[i] = q->data[q->header.size - B + i];
		}
		else {
			for (int i = 1; i <= q->header.size; ++i) q->data[B - i] = q->data[q->header.size - i];
			for (int i = A + 1; i < p->header.size; ++i) q->data[i - A - 1] = p->data[i];
			q->data[p->header.size - A - 1] = {key, p->header.last_child};
			p->header.last_child = p->data[A].child;
			key = p->data[A].key;
		}
		p->header.size = A;
		q->header.size = B;
		return reassign_result::average;
	}
	else {
		// merge
		p->data[p->header.size] = {key, p->header.last_child};
		for (int i = 0; i < q->header.size; ++i) p->data[p->header.size + i + 1] = q->data[i];
		p->header.size += q->header.size + 1;
		p->header.last_child = q->header.last_child;
		return reassign_result::merge;
	}
}

}// namespace kupi
