#include "bpt.h"
#include <iostream>

using bpt = kupi::bpt<int, int, kupi::FileCache>;

void func1() {
	bpt tr("05");
	for (int i = 1; i <= 10000; ++i)
		tr.insert((i * i + 10) % 10007, 0);
	for (int i = 0; i < 10007; ++i)
		tr.erase(i, 0);
}

void func2() {
	bpt tr("05");
	for (int i = 0; i < 10007; ++i) {
		auto r = tr.find(i);
		if (!r.empty()) {
			std::cout << "Wrong at " << i << std::endl;
			break;
		}
	}
}

int main() {
	func1();
	func2();
	return 0;
}
