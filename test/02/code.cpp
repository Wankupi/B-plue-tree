#include "bpt.h"
#include <iostream>
#include <set>
using kupi::bpt;
using std::array, std::cout, std::cin, std::endl;
using ull = unsigned long long;

ull seed = 0x7891238767231231;
ull rnd() {
	seed = (seed << 16) ^ (seed >> 16) ^ (seed * (seed >> 34));
	return seed;
}

int main() {
	int const N = 10, M = 10;
	bpt<ull, int> a;
	std::set<int> s[N];
	ull Key[N];
	for (int i = 0; i < N; ++i) {
		ull key = rnd();
		Key[i] = key;
		for (int j = 0; j < M; ++j) {
			int val = rnd() % 100000;
			a.insert(key, val);
			s[i].insert(val);
		}
	}
	for (int i = 0; i < N; ++i) {
		auto res = a.find(Key[i]);
		cout << "bpt: ";
		for (auto v: res)
			cout << v << ' ';
		cout << endl;
		cout << "set: ";
		for (auto v: s[i])
			cout << v << ' ';
		cout << endl;
	}
	return 0;
}
