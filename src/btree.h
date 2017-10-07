#pragma once

#include "typedef.h"

namespace niffler {

	class BTreeNode {
	public:
		BTreeNode();
		BTreeNode(u32 order);
		~BTreeNode();

		inline bool leaf() const { return num_children_ == 0; }
		inline bool has_max_keys() const { return num_keys_ == max_num_keys(); }
		inline bool has_max_children() const { return num_children_ == max_num_children(); }
		inline u32 num_keys() const { return num_keys_; }
		inline u32 num_children() const { return num_children_; }

		inline constexpr u32 min_num_keys() const { return order_ - 1; }
		inline constexpr u32 max_num_keys() const { return (order_ * 2) - 1; }
		inline constexpr u32 min_num_children() const { return order_; }
		inline constexpr u32 max_num_children() const { return order_ * 2; }

		BTreeNode &get_child(u32 index);
		void insert_child(BTreeNode *child);
		void insert_child(BTreeNode *child, u32 index);

		int get_key(u32 index);
		int get_smallest_key() const;
		void remove_key(u32 index);
		void insert_key(int key);
		void insert_key(int key, u32 index);
		bool linear_find_key(int key);
		bool binary_find_key(int key) const;

		void split_child(u32 index);
		void transfer_largest_keys(BTreeNode &receiver);
		void transfer_largest_children(BTreeNode &receiver);

		inline void set_is_root(bool is_root) { is_root_ = is_root; };
	private:
		bool binary_find_key(int key, i32 start_index, i32 end_index) const;
		bool linear_find_key(int key, BTreeNode &node) const;

		const u32 order_;
		int *keys_;
		u32 num_keys_;
		BTreeNode **children_;
		u32 num_children_;
		bool is_root_ = false;
	};

	class BTree {
	public:
		BTree(u32 order);
		~BTree();

		BTreeNode &root() { return *root_; }
		void insert(int key);
		bool linear_find_key(int key);
	private:
		void insert_non_full(BTreeNode &node, int key);

		const u32 order_;
		u32 height_ = 0;
		BTreeNode *root_;
	};

}