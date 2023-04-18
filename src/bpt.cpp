#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

using Key = int;
using Val = int;

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
		auto operator<=>(pair const &rhs) const {
			return key == rhs.key ? val <=> rhs.val : key <=> rhs.key;
		}
		bool operator==(pair const &rhs) const = default;
	};
	struct base_node_leaf {
		virtual ~base_node_leaf() = default;
	};
	struct leaf;
	struct node : base_node_leaf {
		struct node_data {
			pair key;
			base_node_leaf *child;// children->key <= this->key
		};
		struct node_meta {
			int size;
			base_node_leaf *last_child;
			bool is_leaf;
		};
		constexpr static int M = (BLOCK_SIZE - sizeof(node_meta)) / sizeof(node_data);
		//		constexpr static int M = 5;
		node_meta header;
		node_data data[M];
		// char padding[BLOCK_SIZE - sizeof(header) - sizeof(data)];
		node_data *end() { return data + header.size; }
	};
	struct leaf : base_node_leaf {
		struct leaf_meta {
			int size;
			leaf *last, *next;
		};
		constexpr static int M = (BLOCK_SIZE - sizeof(leaf_meta)) / sizeof(pair);
		//		constexpr static int M = 6;
		leaf_meta header;
		pair data[M];
		// char padding[BLOCK_SIZE - sizeof(int) - sizeof(data)];
		pair *end() { return data + header.size; }
		pair const &back() const { return data[header.size - 1]; }
	};

private:
	base_node_leaf *rt = nullptr;
	bool is_rt_leaf = false;

public:
	void print(node *p) {
		// std::cout << " \033[31m<\033[0m";
		for (int i = 0; i < p->header.size; ++i) {
			if (p->header.is_leaf)
				print(static_cast<leaf *>(p->data[i].child));
			else
				print(static_cast<node *>(p->data[i].child));

			// std::cout << "\033[31m" << p->data[i].key.key << "," << p->data[i].key.val << "\033[0m";
		}
		if (p->header.is_leaf)
			print(static_cast<leaf *>(p->header.last_child));
		else
			print(static_cast<node *>(p->header.last_child));
		// std::cout << "\033[31m>\033[0m ";
	}
	void print(leaf *p) {
		std::cout << " [";
		for (auto cur = p->data; cur != p->data + p->header.size; ++cur)
			std::cout << "(" << cur->key << ',' << cur->val << ")";
		std::cout << "] ";
	}
	void print() {
		if (rt) {
			if (is_rt_leaf)
				print(static_cast<leaf *>(rt));
			else
				print(static_cast<node *>(rt));
			printf("\n");
		}
		else
			printf("empty\n");
	}

private:
	leaf *find_leaf(pair const &x, std::vector<node *> &st);

	// in my insert, because `<=`, the back one won't change, except when spilt
	struct insert_result {
		base_node_leaf *new_node = nullptr;
		pair const *spilt_key = nullptr;// @attention
	};
	/**
	 * @return if not split, return nullptr, else return the new leaf
	 */
	insert_result insert_leaf(leaf *p, pair const &x);
	insert_result insert_node(node *p, bpt::pair const &x, insert_result const &ir);
	void insert_new_root(insert_result const &ir);
};

std::vector<Val> bpt::find(Key const &index) {
	if (!rt) return {};
	pair x{index, {}};
	node::node_data X{x, nullptr};
	node *p = is_rt_leaf ? nullptr : static_cast<node *>(rt);
	leaf *ptr = is_rt_leaf ? static_cast<leaf *>(rt) : nullptr;
	constexpr auto cmp_key_node = [](node::node_data const &A, node::node_data const &B) { return A.key.key < B.key.key; };
	constexpr auto cmp_key_leaf = [](pair const &A, pair const &B) { return A.key < B.key; };
	while (p) {
		auto *k = std::lower_bound(p->data, p->data + p->header.size, X, cmp_key_node);
		auto next = k == p->data + p->header.size ? p->header.last_child : k->child;
		if (p->header.is_leaf) {
			ptr = static_cast<leaf *>(next);
			break;
		}
		p = static_cast<node *>(next);
	}
	std::vector<Val> res;
	while (ptr) {
		auto k = std::lower_bound(ptr->data, ptr->data + ptr->header.size, x, cmp_key_leaf);
		while (k < ptr->data + ptr->header.size && index == k->key) {
			res.emplace_back(k->val);
			++k;
		}
		if (k == ptr->data + ptr->header.size)
			ptr = ptr->header.next;
		else
			break;
	}
	return res;
}

void bpt::insert(Key const &index, Val const &val) {
	pair x{index, val};
	if (!rt) {
		leaf *p = new leaf;
		memset(p, 0, sizeof(leaf));
		p->header.size = 1;
		p->data[0] = x;
		rt = p;
		is_rt_leaf = true;
		return;
	}
	// 1. find leaf
	// 2. add
	// 3. adjust up
	std::vector<node *> st;
	leaf *ptr = find_leaf(x, st);
	auto ir = insert_leaf(ptr, x);
	if (!ir.new_node) return;
	for (auto cur = st.rbegin(); ir.new_node && cur != st.rend(); ++cur)
		ir = insert_node(*cur, x, ir);
	if (ir.new_node)
		insert_new_root(ir);
}

bpt::leaf *bpt::find_leaf(const bpt::pair &x, std::vector<node *> &st) {
	if (is_rt_leaf) return static_cast<leaf *>(rt);
	node::node_data X{x, nullptr};
	node *p = static_cast<node *>(rt);
	constexpr auto cmp_key_node = [](node::node_data const &A, node::node_data const &B) { return A.key < B.key; };
	while (true) {
		st.push_back(p);
		auto k = std::lower_bound(p->data, p->data + p->header.size, X, cmp_key_node);
		auto next = k == p->data + p->header.size ? p->header.last_child : k->child;
		if (p->header.is_leaf)
			return static_cast<leaf *>(next);
		else
			p = static_cast<node *>(next);
	}
}

bpt::insert_result bpt::insert_leaf(bpt::leaf *p, bpt::pair const &x) {
	auto k = std::lower_bound(p->data, p->end(), x);
	if (k != p->end() && *k == x) return {nullptr};
	pair last = k == p->end() ? x : p->back();// ==x might happen when p is the back leaf on the tree
	for (auto cur = p->data + std::min(p->header.size, leaf::M - 1); cur > k; --cur)
		*cur = *(cur - 1);
	if (p->header.size < leaf::M || k != p->end())
		*k = x;
	if (++(p->header.size) <= leaf::M) return {nullptr};
	constexpr int A = (leaf::M + 2) / 2, B = leaf::M + 1 - A;
	leaf *q = new leaf;
	q->header = leaf::leaf_meta{B, p, p->header.next};
	p->header = leaf::leaf_meta{A, p->header.last, q};
	for (int i = A; i < leaf::M; ++i)
		q->data[i - A] = p->data[i];
	q->data[B - 1] = last;
	return {q, &p->back()};
}

bpt::insert_result bpt::insert_node(node *p, bpt::pair const &x, insert_result const &ir) {
	constexpr auto cmp_key_node = [](node::node_data const &A, node::node_data const &B) { return A.key < B.key; };
	auto k = std::lower_bound(p->data, p->data + p->header.size, node::node_data{x, nullptr}, cmp_key_node);
	node::node_data X{*ir.spilt_key, nullptr};
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
	if (++(p->header.size) <= node::M) return {nullptr};
	constexpr int A = (node::M + 1) / 2, B = node::M - A;
	node *q = new node;
	q->header = node::node_meta{B, p->header.last_child, p->header.is_leaf};
	p->header = node::node_meta{A, p->data[A].child, p->header.is_leaf};
	for (int i = A + 1; i < node::M; ++i)
		q->data[i - A - 1] = p->data[i];
	q->data[B - 1] = last;
	// return reference is not very safe... just do it in memory for convenience
	return {q, &p->data[A].key};
}

void bpt::insert_new_root(insert_result const &ir) {
	node *q = new node;
	q->header = node::node_meta{1, ir.new_node, is_rt_leaf};
	q->data[0].key = *ir.spilt_key;
	q->data[0].child = rt;
	rt = q;
	is_rt_leaf = false;
}

using std::cout, std::cin, std::endl;
template<typename T>
std::ostream &operator<<(std::ostream &os, std::vector<T> const &v) {
	os << "[ ";
	for (auto const &x: v) os << x << ' ';
	return os << "]" << std::flush;
}
#include <chrono>
#include <random>
#include <set>

template<typename Key, typename Val>
std::ostream &operator<<(std::ostream &os, std::set<std::pair<Key, Val>> const &v) {
	for (auto [key, val]: v) {
		os << "(" << key << "," << val << ")";
	}
	return os << endl;
}


std::vector<int> find(std::set<std::pair<int, int>> const &s, int key) {
	std::vector<int> res;
	auto p = s.lower_bound({key, -0x7fffffff});
	while (p != s.end() && p->first == key)
		res.push_back((p++)->second);
	return res;
}

void test_std();
void test_input();

int main() {
	test_std();
	return 0;
}

void test_std() {
	std::default_random_engine e(std::chrono::system_clock::now().time_since_epoch().count());
	//	std::default_random_engine e(12345);
	int V = 50;
	std::uniform_int_distribution opt(1, 2), x(0, V), y(0, V);
	int n = 5000;
	for (int T = 1; T <= 50000; ++T) {
		if (T % 1000 == 0) std::cout << "Case " << T << endl;
		bpt tr;
		std::set<std::pair<int, int>> m;
		bool fault = false;
		std::vector<std::pair<int, std::pair<int, int>>> ops;
		std::vector<int> v1, v2;
		for (int i = 1; i <= n; ++i) {
			int o = opt(e);
			int key = x(e), value = y(e);
			if (o == 1) {
				ops.push_back({o, {key, value}});
				tr.insert(key, value);
				m.insert({key, value});
			}
			else {
				ops.push_back({o, {key, 0}});
				v1 = tr.find(key);
				v2 = find(m, key);
				if (v1 != v2) {
					cout << "At Step " << i << endl;
					fault = true;
					break;
				}
			}
		}
		if (fault) {
			for (auto [o, p]: ops) {
				auto [key, value] = p;
				if (o == 1)
					cout << "i " << key << ' ' << value << endl;
				else
					cout << "q " << key << endl;
			}
			cout << v1 << endl
				 << v2 << endl;
			break;
		}
	}
}

void test_input() {
	bpt tr;
	std::set<std::pair<int, int>> m;
	char opt[20];
	int key, value;
	while (cin >> opt >> key) {
		static int step = 0;
		cout << "Step " << ++step << endl;
		if (*opt == 'i') {
			cin >> value;
			tr.insert(key, value);
			m.insert({key, value});
		}
		else {
			auto v1 = tr.find(key), v2 = find(m, key);
			std::cout << v1 << endl
					  << v2 << std::endl;
		}
		tr.print();
		cout << m;
	}
}
