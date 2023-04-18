#include <algorithm>
#include <array>
#include <assert.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <ranges>
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
		//		constexpr static int M = 2;
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
		//		constexpr static int M = 3;
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
		assert(p->header.size <= node::M);
		std::cout << " \033[31m<\033[0m";
		for (int i = 0; i < p->header.size; ++i) {
			if (p->header.is_leaf)
				print(static_cast<leaf *>(p->data[i].child));
			else
				print(static_cast<node *>(p->data[i].child));

			std::cout << "\033[31m" << p->data[i].key.key /*<< "," << p->data[i].key.val */ << "\033[0m";
		}
		if (p->header.is_leaf)
			print(static_cast<leaf *>(p->header.last_child));
		else
			print(static_cast<node *>(p->header.last_child));
		std::cout << "\033[31m>\033[0m ";
	}
	void print(leaf *p) {
		std::cout << " [ ";
		for (auto cur = p->data; cur != p->data + p->header.size; ++cur)
			std::cout << "(" << cur->key << ',' << cur->val << ")";
		//			std::cout << cur->key << " ";
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
	leaf *find_leaf(pair const &x, std::vector<std::pair<node *, node::node_data *>> &st);

	// in my insert, because `<=`, the back one won't change, except when spilt
	struct insert_result {
		base_node_leaf *new_node = nullptr;
		pair const *spilt_key = nullptr;// @attention
	};
	/**
	 * @return if not split, return nullptr, else return the new leaf
	 */
	insert_result insert_leaf(leaf *p, pair const &x);
	insert_result insert_node(node *p, node::node_data *k, insert_result const &ir);
	void insert_new_root(insert_result const &ir);

	struct erase_result {
		pair const *update_key = nullptr;
		bool is_short = false;
	};
	erase_result erase_leaf(leaf *p, pair const &x);
	/**
	 * @brief called when k is short, try to reassign its content.
	 * @param p
	 * @param k
	 * @return
	 */
	bool erase_node(node *p, node::node_data *k);

	enum class reassign_result { average,
								 merge };
	/**
	 * @param p,q : the two nodes(leave) to be averaged or merged
	 * @param key : the mid key spilt the two nodes
	 */
	reassign_result reassign(base_node_leaf *p, pair &key, base_node_leaf *q, bool is_leaf) {
		return is_leaf ? reassign_leaf(static_cast<leaf *>(p), key, static_cast<leaf *>(q)) : reassign_node(static_cast<node *>(p), key, static_cast<node *>(q));
	}
	reassign_result reassign_leaf(leaf *p, pair &key, leaf *q);
	reassign_result reassign_node(node *p, pair &key, node *q);
	//	erase_result erase_node(node *p, node::node_data *k, erase_result &er);
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
	std::vector<std::pair<node *, node::node_data *>> st;
	leaf *ptr = find_leaf(x, st);
	auto ir = insert_leaf(ptr, x);
	if (!ir.new_node) return;
	for (auto cur = st.rbegin(); ir.new_node && cur != st.rend(); ++cur)
		ir = insert_node(cur->first, cur->second, ir);
	if (ir.new_node)
		insert_new_root(ir);
}

bpt::leaf *bpt::find_leaf(const pair &x, std::vector<std::pair<node *, node::node_data *>> &st) {
	if (is_rt_leaf) return static_cast<leaf *>(rt);
	node::node_data X{x, nullptr};
	node *p = static_cast<node *>(rt);
	constexpr auto cmp_key_node = [](node::node_data const &A, node::node_data const &B) { return A.key < B.key; };
	while (true) {
		auto k = std::lower_bound(p->data, p->data + p->header.size, X, cmp_key_node);
		st.push_back({p, k});
		auto next = k == p->data + p->header.size ? p->header.last_child : k->child;
		if (p->header.is_leaf)
			return static_cast<leaf *>(next);
		else
			p = static_cast<node *>(next);
	}
}

bpt::insert_result bpt::insert_leaf(leaf *p, pair const &x) {
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

bpt::insert_result bpt::insert_node(node *p, node::node_data *k, insert_result const &ir) {
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

void bpt::erase(const Key &index, const Val &val) {
	if (!rt) return;
	pair x{index, val};
	std::vector<std::pair<node *, node::node_data *>> st;
	leaf *ptr = find_leaf(x, st);
	auto er = erase_leaf(ptr, x);
	// deal with the change of key first
	if (er.update_key) {
		for (auto &cur: std::ranges::reverse_view(st))
			if (cur.second != cur.first->data + cur.first->header.size) {
				cur.second->key = *er.update_key;
				break;
			}
	}
	for (auto cur = st.rbegin(); er.is_short && cur != st.rend(); ++cur)
		er.is_short = erase_node(cur->first, cur->second);
	if (er.is_short) {
		if (is_rt_leaf) return;
		node *RT = static_cast<node *>(rt);
		if (RT->header.size) return;
		rt = RT->header.last_child;
		is_rt_leaf = RT->header.is_leaf;
		delete RT;
	}
}

bpt::erase_result bpt::erase_leaf(leaf *p, const pair &x) {
	auto k = std::lower_bound(p->data, p->end(), x);
	if (k == p->end() || *k != x) return {};
	for (auto cur = k + 1; cur < p->end(); ++cur)
		*(cur - 1) = *cur;
	--(p->header.size);
	pair const *back = (k == p->end() ? &p->back() : nullptr);
	return {back, p->header.size < (leaf::M + 1) / 2};
}

bool bpt::erase_node(node *p, node::node_data *k) {
	auto get_size = [is_leaf = p->header.is_leaf](base_node_leaf *ptr) {
		if (!ptr) return 0;
		return is_leaf ? static_cast<leaf *>(ptr)->header.size : static_cast<node *>(ptr)->header.size;
	};
	auto pre = k == p->data ? nullptr : (k - 1)->child;
	auto next = (k == p->end() ? nullptr : (k == p->end() - 1 ? p->header.last_child : (k + 1)->child));
	//	if (!pre && !next) // this line won't happen
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
	base_node_leaf *to_release = nullptr;
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
	if (p->header.is_leaf) delete static_cast<leaf *>(to_release);
	else
		delete static_cast<node *>(to_release);
	return p->header.size < (node::M + 1) / 2;
}

bpt::reassign_result bpt::reassign_leaf(leaf *p, pair &key, leaf *q) {
	if (p->header.size + q->header.size > leaf::M) {
		// average
		int A = (p->header.size + q->header.size + 1) / 2, B = p->header.size + q->header.size - A;
		assert(A != p->header.size);
		assert(B != q->header.size);
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
		if (p->header.next) p->header.next->header.last = p;
		return reassign_result::merge;
	}
}

bpt::reassign_result bpt::reassign_node(node *p, pair &key, node *q) {
	if (p->header.size + q->header.size >= node::M) {
		// average
		int A = (p->header.size + q->header.size + 1) / 2, B = p->header.size + q->header.size - A;
		assert(A != p->header.size);
		assert(B != q->header.size);
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


void test_simple();
void test_std();
void test_input();

int main() {
	test_std();
	//	test_simple();
	//	test_input();
	return 0;
}

void test_simple() {
	bpt tr;
	for (int i = 1; i <= 20; ++i) tr.insert(i, 0);
	tr.print();
	for (int i = 1; i <= 10; ++i) {
		int k = (i + 3) % 10 + 1;
		cout << "erase " << k << endl;
		tr.erase(k, 0);
		tr.print();
	}
}

std::ostream &operator<<(std::ostream &os, std::pair<int, std::pair<int, int>> const &a) {
	os << "idq"[a.first - 1] << ' ' << a.second.first;
	if (a.first != 3) os << ' ' << a.second.second;
	return os;
}

void print(std::vector<std::pair<int, std::pair<int, int>>> const &ops) {
	for (auto &a: ops)
		cout << a << endl;
}


void test_std() {
	std::default_random_engine e(std::chrono::system_clock::now().time_since_epoch().count());
	// std::default_random_engine e(12345);
	int V = 500;
	std::uniform_int_distribution opt(1, 3), x(0, V), y(0, V);
	int n = 2000;
	for (int T = 1; T <= 50000; ++T) {
		if (T % 1000 == 0) std::cout << "Case " << T << endl;
		bpt tr;
		std::set<std::pair<int, int>> m;
		bool fault = false;
		std::vector<std::pair<int, std::pair<int, int>>> ops;
		std::vector<int> v1, v2;
		try {
			for (int i = 1; i <= n; ++i) {
				int o = opt(e);
				int key = x(e);
				//				int value = y(e);
				int value = 0;
				if (o == 1) {
					ops.push_back({o, {key, value}});
					tr.insert(key, value);
					m.insert({key, value});
				}
				else if (o == 2) {
					ops.push_back({o, {key, value}});
					tr.erase(key, value);
					m.erase({key, value});
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
		} catch (...) {
			fault = true;
		}
		if (fault) {
			for (auto [o, p]: ops) {
				auto [key, value] = p;
				if (o == 1)
					cout << "i " << key << ' ' << value << endl;
				else if (o == 2)
					cout << "d " << key << ' ' << value << endl;
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
		if (*opt == 'e') break;
		static int step = 0;
		cout << "Step " << ++step << endl;
		if (*opt == 'i') {
			cin >> value;
			tr.insert(key, value);
			m.insert({key, value});
		}
		else if (*opt == 'd') {
			cin >> value;
			tr.erase(key, value);
			m.erase({key, value});
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
