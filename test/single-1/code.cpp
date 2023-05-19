#include "bpt.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <string>
using namespace std;

template class kupi::bpt<int, int, kupi::FileCache>;

using bpt = kupi::bpt<int, int, kupi::FileCache>;

void test_std();
void test_input();

int main() {
	//	test_input();
	test_std();
	return 0;
}


void test_std() {
	std::default_random_engine e(std::chrono::system_clock::now().time_since_epoch().count());
	//	std::default_random_engine e(12345);
	int V = 500;
	std::uniform_int_distribution opt(1, 4), x(0, V), y(0, V);
	int n = 10000;

	for (int T = 1; T <= 5000; ++T) {
		std::filesystem::remove("bpt.leave");
		std::filesystem::remove("bpt.leave.trash");
		std::filesystem::remove("bpt.nodes");
		std::filesystem::remove("bpt.nodes.trash");
		if (T % 100 == 0)
			std::cout << "Case " << T << endl;
		auto beg_time = std::chrono::system_clock::now();
		bpt tr("bpt");
		std::map<int, int> m;
		bool fault = false;
		std::vector<std::pair<int, std::pair<int, int>>> ops;

		for (int i = 1; i <= n; ++i) {
			int o = opt(e);
			int key = x(e);
			int value = x(e);
			if (o == 4) o = 1;
			if (o == 1) {
				ops.push_back({o, {key, value}});
				tr.insert(key, value);
				m.insert({key, value});
			}
			else if (o == 2) {
				ops.push_back({o, {key, 0}});
				tr.erase(key);
				m.erase(key);
			}
			else if (o == 3) {
				ops.push_back({o, {key, 0}});
				auto p1 = tr.find(key);
				auto p2 = m.find(key);
				if ((int(p1 == tr.end()) ^ int(p2 == m.end())) ||// not same
					(p1 != tr.end() && p1->second != p2->second)) {
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
				else if (o == 2 || o == 4)
					cout << "d " << key << endl;
				else if (o == 3)
					cout << "q " << key << endl;
			}
			break;
		}

		auto end_time = std::chrono::system_clock::now();
		auto dur = end_time - beg_time;
		//		std::cout << "time: " << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count() << "ms" << std::endl;
	}
}

void test_input() {
	std::filesystem::remove("bpt.leave");
	std::filesystem::remove("bpt.leave.trash");
	std::filesystem::remove("bpt.nodes");
	std::filesystem::remove("bpt.nodes.trash");

	bpt tr("bpt");
	std::map<int, int> m;
	char opt[20];
	int key, value;
	while (cin >> opt) {
		if (*opt == 'e') break;
		cin >> key;
		static int step = 0;
		cout << "Step " << ++step << " \t[ \033[31m" << *opt << ' ' << key << "\033[0m ]" << endl;
		if (*opt == 'i') {
			cin >> value;
			tr.insert(key, value);
			m.insert({key, value});
		}
		else if (*opt == 'd') {
			cin >> value;
			tr.erase(key);
			m.erase(key);
		}
		else {
			auto p1 = tr.find(key);
			auto p2 = m.find(key);
			std::cout << (p1 == tr.end() ? string("nothing") : to_string(p1->second)) << ' '
					  << (p2 == m.end() ? string("nothing") : to_string(p2->second)) << std::endl;
		}
		//		tr.print();
		//		cout << m;
	}
}
