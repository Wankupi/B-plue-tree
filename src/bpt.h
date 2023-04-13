#pragma
#ifndef B_TREE_BPT_H
#define B_TREE_BPT_H
#include "bpt_exception.h"
#include <memory>
#include <vector>
/**
 * @brief a multimap
 * @tparam Key type of key
 * @tparam Val type of value
 * @tparam Alloc type of allocator
 * @attention one Key may have various values, but {key, value} is considered distinct.
 */
template<typename Key, typename Val, template<typename Type> class Alloc = std::allocator>
class bpt {
public:
	bool insert(Key const &index, Val const &val);
	bool erase(Key const &index, Val const &val);
	std::vector<Val> find(Key const &index);
private:
};

#endif//B_TREE_BPT_H
