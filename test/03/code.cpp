#define LOCAL_TEST
#include "bpt.h"
#include <array>
#include <chrono>
#include <iostream>
#include <random>
#include <set>
#include <string>
using namespace std;

using bpt = kupi::bpt<int, int, kupi::FileCache>;

void test_simple();
void test_std();
void test_input();


std::ostream &operator<<(std::ostream &os, std::pair<int, std::pair<int, int>> const &a) {
	os << "idq"[a.first - 1] << ' ' << a.second.first;
	if (a.first != 3) os << ' ' << a.second.second;
	return os;
}
template<typename T>
std::ostream &operator<<(std::ostream &os, std::vector<T> const &v) {
	os << "[ ";
	for (auto const &x: v) os << x << ' ';
	return os << "]" << std::flush;
}

void print(std::vector<std::pair<int, std::pair<int, int>>> const &ops) {
	for (auto &a: ops)
		cout << a << endl;
}

std::vector<int> find(std::set<std::pair<int, int>> const &s, int key) {
	std::vector<int> res;
	auto p = s.lower_bound({key, -0x7fffffff});
	while (p != s.end() && p->first == key)
		res.push_back((p++)->second);
	return res;
}

template<typename Key, typename Val>
std::ostream &operator<<(std::ostream &os, std::set<std::pair<Key, Val>> const &v) {
	for (auto [key, val]: v) {
		os << "(" << key << "," << val << ")";
	}
	return os << endl;
}


int main() {
	// test_std();
	// test_simple();
	test_input();
	return 0;
}

void test_simple() {
	bpt tr("03");
	for (int i = 1; i <= 20; ++i) tr.insert(i, 0);
	//	tr.print();
	for (int i = 1; i <= 10; ++i) {
		int k = (i + 3) % 10 + 1;
		cout << "erase " << k << endl;
		tr.erase(k, 0);
		//		tr.print();
	}
}

void test_std() {
	//	std::default_random_engine e(std::chrono::system_clock::now().time_since_epoch().count());
	std::default_random_engine e(12345);
	int V = 200;
	std::uniform_int_distribution opt(1, 4), x(0, V), y(0, V);
	int n = 5000;
	for (int T = 1; T <= 5000; ++T) {
		//		if (T % 1000 == 0)
		std::cout << "Case " << T << endl;
		bpt tr("03");
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
				else if (o == 2 || o == 4) {
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
		} catch (kupi::bpt_exception const &e) {
			fault = true;
			std::cout << e.what() << std::endl;
		}
		if (fault) {
			for (auto [o, p]: ops) {
				auto [key, value] = p;
				if (o == 1)
					cout << "i " << key << ' ' << value << endl;
				else if (o == 2 || o == 4)
					cout << "d " << key << ' ' << value << endl;
				else if (o == 3)
					cout << "q " << key << endl;
			}
			cout << v1 << endl
				 << v2 << endl;
			break;
		}
	}
}

void test_input() {
	bpt tr("03");
	// kupi::bpt<int, int> tr;
	std::set<std::pair<int, int>> m;
	char opt[20];
	int key, value;
	while (cin >> opt >> key) {
		if (*opt == 'e') break;
		static int step = 0;
		cout << "Step " << ++step << " \t[ \033[31m" << *opt << ' ' << key << "\033[0m ]" << endl;
		if (*opt == 'i') {
			cin >> value;
			tr.insert(key, value);
			m.insert({key, value});
		}
		else if (*opt == 'd') {
			cin >> value;
			tr.erase(key, value);
			m.erase({key, value});
			// tr.debug();
		}
		else {
			auto v1 = tr.find(key), v2 = find(m, key);
			std::cout << v1 << endl
					  << v2 << std::endl;
			// if (v1 != v2) break;
		}
		tr.print();
		cout << m;
	}
}
