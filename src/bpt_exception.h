#ifndef B_TREE_BPT_EXCEPTION_H
#define B_TREE_BPT_EXCEPTION_H

#include <exception>

struct bpt_exception : std::exception {
    bpt_exception(const char *msg) : msg(msg) {}
    const char * what() const noexcept override { return msg; }
private:
    const char *const msg;
};

#endif  B_TREE_BPT_EXCEPTION_H
