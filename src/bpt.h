#pragma
#include "bpt_exception.h"
#include <memory>
#include <vector>

namespace kupi {

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
	constexpr static int BLOCK_SIZE = 4096;
	struct pair {
		Key key;
		Val val;
		friend bool operator<(pair const &lhs, pair const &rhs) {
			return lhs.key == rhs.key ? lhs.val < rhs.val : lhs.key < rhs.key;
		}
	};
	struct node {
		struct node_meta {
			int size;
			int first_child;
			bool is_leaf;
		};
		struct node_data {
			pair key;
			int child;
		};
		constexpr static int M = (BLOCK_SIZE - sizeof(node_meta)) / sizeof(node_data);
		node_meta header;
		node_data data[M];
		// char padding[BLOCK_SIZE - sizeof(header) - sizeof(data)];
	};
	struct leaf {
		struct leaf_meta {
			int size;
		};
		constexpr static int M = (BLOCK_SIZE - sizeof(leaf_meta)) / sizeof(pair);
		leaf_meta header;
		pair data[M];
		// char padding[BLOCK_SIZE - sizeof(int) - sizeof(data)];
	};

private:
	node *rt;
};

}// namespace kupi
