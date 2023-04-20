#pragma once
#include <exception>

namespace kupi {
struct bpt_exception : std::exception {
	bpt_exception(const char *msg) : msg(msg) {}
	const char *what() const noexcept override { return msg; }

private:
	const char *const msg;
};
}// namespace kupi
