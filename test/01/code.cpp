#include "bpt.h"
#include <array>
using std::array;
using kupi::bpt;
int main() {
	bpt<long long, int> a;
	bpt<unsigned char, long long> b;
	bpt<array<char, 10>, int> c;
	bpt<int, array<char, 10>> d;
	return 0;
}
