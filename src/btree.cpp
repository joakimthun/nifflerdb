#include "btree.h"

#include <assert.h>

namespace niffler {

	BTreeNode::BTreeNode()
		:
		order_(0),
		num_keys_(0),
		keys_(nullptr),
		num_children_(0),
		children_(nullptr)
	{
	}

	BTreeNode::BTreeNode(u32 order)
		:
		order_(order),
		num_keys_(0),
		num_children_(0)
	{
		assert(order_ > 0);
		keys_ = new int[max_num_keys()];
		children_ = new BTreeNode*[max_num_children()];
	}

	BTreeNode::~BTreeNode()
	{
		if (keys_ != nullptr)
		{
			delete[] keys_;
		}

		if (children_ != nullptr)
		{
			for (auto i = 0u; i < num_children_; i++)
			{
				delete children_[i];
			}

			delete[] children_;
		}
	}

	BTreeNode &BTreeNode::get_child(u32 index)
	{
		assert(index < num_children_);
		return *children_[index];
	}

	void BTreeNode::insert_child(BTreeNode *child)
	{
		assert(!has_max_children());

		for (auto i = 0u; i < num_children_; i++)
		{
			if (child->get_smallest_key() < get_child(i).get_smallest_key())
			{
				insert_child(child, i);
				return;
			}
		}

		// The new child is the "largest", insert it at the end
		insert_child(child, num_children_);
	}

	void BTreeNode::insert_child(BTreeNode *child, u32 index)
	{
		// Add one to enable inserts at the end of the children array
		assert(index < (num_children_ + 1));
		assert(!has_max_children());

		// Use a signed index to prevent i from wrapping on 0--
		for (auto i = static_cast<i32>(num_children_) - 1; i >= static_cast<i32>(index); i--)
		{
			children_[i + 1] = children_[i];
		}

		children_[index] = child;
		num_children_++;
	}

	int BTreeNode::get_key(u32 index)
	{
		assert(index < num_keys_);
		return keys_[index];
	}

	int BTreeNode::get_smallest_key() const
	{
		if (num_keys_ > 0)
			return keys_[0];

		return 0;
	}

	void BTreeNode::remove_key(u32 index)
	{
		assert(index < num_keys_);
		for (auto i = index; i < num_keys_; i++)
		{
			keys_[i] = keys_[i + 1];
		}

		num_keys_--;
	}

	void BTreeNode::insert_key(int key)
	{
		assert(!has_max_keys());

		for (auto i = 0u; i < num_keys_; i++)
		{
			if (key < keys_[i])
			{
				insert_key(key, i);
				return;
			}
		}

		// The new key is the largest, insert it at the end
		insert_key(key, num_keys_);
	}

	void BTreeNode::insert_key(int key, u32 index)
	{
		// Add one to enable inserts at the end of the keys array
		assert(index < (num_keys_ + 1));
		assert(!has_max_keys());

		// Use a signed index to prevent i from wrapping on 0--
		for (auto i = static_cast<i32>(num_keys_) - 1; i >= static_cast<i32>(index); i--)
		{
			keys_[i + 1] = keys_[i];
		}

		keys_[index] = key;
		num_keys_++;
	}

	bool BTreeNode::linear_find_key(int key)
	{
		return linear_find_key(key, *this);
	}

	bool BTreeNode::binary_find_key(int key) const
	{
		return binary_find_key(key, 0, num_keys_ - 1);
	}

	void BTreeNode::split_child(u32 index)
	{
		assert(index < num_children_);
		auto new_node = new BTreeNode(order_);
		auto &node_to_split = get_child(index);

		// Move keys greater than the median to the new node
		node_to_split.transfer_largest_keys(*new_node);

		if (!node_to_split.leaf())
		{
			// Move children greater than the median to the new node
			node_to_split.transfer_largest_children(*new_node);
		}

		auto median_key_index = node_to_split.min_num_children() - 1;
		auto median_key = node_to_split.get_key(median_key_index);
		node_to_split.remove_key(median_key_index);

		insert_child(new_node, index + 1);
		insert_key(median_key, index);
		//height_++;
	}

	void BTreeNode::transfer_largest_keys(BTreeNode &receiver)
	{
		assert(has_max_keys());

		for (auto i = order_; i < max_num_keys(); i++)
		{
			receiver.insert_key(keys_[i]);
			keys_[i] = 0;
			num_keys_--;
		}
	}

	void BTreeNode::transfer_largest_children(BTreeNode &receiver)
	{
		assert(has_max_children());

		for (auto i = order_; i < max_num_children(); i++)
		{
			receiver.insert_child(children_[i]);
			children_[i] = nullptr;
			num_children_--;
		}
	}

	bool BTreeNode::binary_find_key(int key, i32 start_index, i32 end_index) const
	{
		if (end_index >= start_index)
		{
			assert(num_keys_ > 0);
			assert(num_keys_ > static_cast<u32>(start_index));
			assert(num_keys_ > static_cast<u32>(end_index));

			auto mid_index = start_index + (end_index - start_index) / 2;
			auto mid_key = keys_[mid_index];

			if (key == mid_key)
				return true;

			if (mid_key > key)
				return binary_find_key(key, start_index, mid_index - 1);

			return binary_find_key(key, mid_index + 1, end_index);
		}

		return false;
	}

	bool BTreeNode::linear_find_key(int key, BTreeNode &node) const
	{
		auto i = 0u;
		while (i < node.num_keys() && key > node.get_key(i))
			i++;

		if (i < node.num_keys() && key == node.get_key(i))
			return true;

		if(node.leaf())
			return false;

		return linear_find_key(key, node.get_child(i));
	}

	BTree::BTree(u32 order)
		:
		order_(order)
	{
		root_ = new BTreeNode(order_);
	}

	BTree::~BTree()
	{
		if (root_ != nullptr)
			delete root_;
	}

	void BTree::insert(int key)
	{
		if (root_->has_max_keys())
		{
			auto new_node = new BTreeNode(order_);
			new_node->insert_child(root_);
			root_->set_is_root(false);
			root_ = new_node;
			new_node->set_is_root(true);
			root_->split_child(0);
			insert_non_full(*root_, key);
		}
		else
		{
			insert_non_full(*root_, key);
		}
	}

	bool BTree::linear_find_key(int key)
	{
		return root_->linear_find_key(key);
	}

	void BTree::insert_non_full(BTreeNode &node, int key)
	{
		// Leaf node, insert the key here
		if (node.leaf())
		{
			node.insert_key(key);
		}
		// Find which child to "possibly" insert the new key into
		else
		{
			assert(node.num_keys() > 0);
			auto target_child_index = node.num_keys();
			while (target_child_index >= 0 && key < node.get_key(target_child_index - 1))
			{
				target_child_index -= 1;
			}

			// If the target child is full we need to split it before stepping into it
			if (node.get_child(target_child_index).has_max_keys())
			{
				node.split_child(target_child_index);
				// Should we put the new key in the right node of the split?
				if (key > node.get_key(target_child_index))
				{
					target_child_index += 1;
				}
			}

			insert_non_full(node.get_child(target_child_index), key);
		}
	}

}
