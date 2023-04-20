#include "bpt.h"
#include <array>
#include <cstring>
#include <iostream>
#include <string>

template<std::size_t N>
struct String : public std::array<char, N> {
	using std::array<char, N>::array;
	String() {
		memset(this->data(), 0, N);
	}
	String(std::string const &str) : String() {
		memcpy(this->data(), str.data(), std::min(N, str.size()));
	}
	String(const char *str) : String() {
		memcpy(this->data(), str, std::min(N, strlen(str)));
	}
	explicit operator std::string() const {
		if (this->back()) {
			std::string s(N, 0);
			memcpy(s.data(), this->data(), N);
			return s;
		}
		else
			return std::string(this->data());
	}
	bool allzero() const {
		for (int i = 0; i < N; ++i)
			if ((*this)[i]) return false;
		return true;
	}
};

template<std::size_t N>
std::ostream &operator<<(std::ostream &os, String<N> const &str) {
	if (str.back())
		os.write(str.data(), N);
	else
		os << str.data();
	return os;
}

int main() {
	kupi::bpt<String<64>, int, kupi::FileCache> a("bpt");
	int n = 0;
	std::string opt, key;
	int val;
	std::cin >> n;
	for (int t = 1; t <= n; ++t) {
		std::cin >> opt >> key;
		if (opt[0] != 'f') std::cin >> val;
		if (opt[0] == 'i') a.insert(key, val);
		else if (opt[0] == 'd') a.erase(key, val);
		else {
			auto r = a.find(key);
			for (auto v : r)
				std::cout << v << ' ';
			if (r.empty()) std::cout << "null";
			std::cout << '\n';
		}
	}
	return 0;
}
