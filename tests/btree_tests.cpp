//#include <gtest\gtest.h>
//#include <string>
//#include <memory>
//
//#include "btree.h"
//
//using namespace niffler;
//
//struct ValidateTreeResult
//{
//	ValidateTreeResult(bool valid, const std::string &message) : valid(valid), message(message) {}
//	bool valid;
//	std::string message;
//};
//
//ValidateTreeResult validate_tree(BTreeNode &node, bool root)
//{
//	if (!root)
//	{
//		if (!node.leaf())
//		{
//			if (node.num_children() < node.min_num_children())
//				return ValidateTreeResult(false, "num_children_ < min_num_children()");
//			if (node.num_children() > node.max_num_children())
//				return ValidateTreeResult(false, "num_children_ > max_num_children()");
//		}
//
//		if (node.num_keys() < node.min_num_keys())
//			return ValidateTreeResult(false, "num_keys_ < min_num_keys()");
//		if (node.num_keys() > node.max_num_keys())
//			return ValidateTreeResult(false, "num_keys_ > max_num_keys()");
//
//	}
//
//	if (!node.leaf())
//	{
//		for (auto ki = 0u; ki < node.num_keys(); ki++)
//		{
//			const auto key = node.get_key(ki);
//
//			auto& smaller = node.get_child(ki);
//			for (auto ski = 0u; ski < smaller.num_keys(); ski++)
//			{
//				const auto s_key = smaller.get_key(ski);
//				if (s_key >= key)
//					return ValidateTreeResult(false, "s_key >= key");
//			}
//
//			if ((ki + 1) < node.num_children())
//			{
//				auto& larger = node.get_child(ki + 1);
//				for (auto ski = 0u; ski < larger.num_keys(); ski++)
//				{
//					const auto l_key = larger.get_key(ski);
//					if (l_key <= key)
//						return ValidateTreeResult(false, "l_key <= key");
//				}
//			}
//		}
//	}
//
//	for (auto i = 0u; i < node.num_children(); i++)
//	{
//		auto r = validate_tree(node.get_child(i), false);
//		if (!r.valid)
//			return r;
//	}
//
//	return ValidateTreeResult(true, "");
//}
//
//std::unique_ptr<BTree> fill_tree(u32 tree_order, int num_keys)
//{
//	auto tree = std::make_unique<BTree>(tree_order);
//	for (auto key = 1; key <= num_keys; key++)
//	{
//		tree->insert(key);
//	}
//
//	return tree;
//}
//
//ValidateTreeResult validate_insert_keys(u32 tree_order, int num_keys)
//{
//	auto tree = BTree(tree_order);
//	for (auto key = 1; key <= num_keys; key++)
//	{
//		tree.insert(key);
//		auto result = validate_tree(tree.root(), true);
//		if (!result.valid)
//			return result;
//	}
//
//	return ValidateTreeResult(true, "");
//}
//
//TEST(BTREE_NODE, ORDER_CALCULATIONS)
//{
//	auto node1 = BTreeNode(10);
//	EXPECT_EQ(9, node1.min_num_keys());
//	EXPECT_EQ(19, node1.max_num_keys());
//	EXPECT_EQ(10, node1.min_num_children());
//	EXPECT_EQ(20, node1.max_num_children());
//
//	auto node2 = BTreeNode(1000);
//	EXPECT_EQ(999, node2.min_num_keys());
//	EXPECT_EQ(1999, node2.max_num_keys());
//	EXPECT_EQ(1000, node2.min_num_children());
//	EXPECT_EQ(2000, node2.max_num_children());
//}
//
//TEST(BTREE_NODE, TRANSFER_LARGEST_KEYS)
//{
//	auto node = BTreeNode(10);
//	node.insert_key(1);
//	node.insert_key(2);
//	node.insert_key(3);
//	node.insert_key(4);
//	node.insert_key(5);
//	node.insert_key(6);
//	node.insert_key(7);
//	node.insert_key(8);
//	node.insert_key(9);
//	node.insert_key(10);
//	node.insert_key(11);
//	node.insert_key(12);
//	node.insert_key(13);
//	node.insert_key(14);
//	node.insert_key(15);
//	node.insert_key(16);
//	node.insert_key(17);
//	node.insert_key(18);
//	node.insert_key(19);
//
//	auto receiver= BTreeNode(10);
//	node.transfer_largest_keys(receiver);
//
//	EXPECT_EQ(9, receiver.num_keys());
//	EXPECT_EQ(11, receiver.get_key(0));
//	EXPECT_EQ(12, receiver.get_key(1));
//	EXPECT_EQ(13, receiver.get_key(2));
//	EXPECT_EQ(14, receiver.get_key(3));
//	EXPECT_EQ(15, receiver.get_key(4));
//	EXPECT_EQ(16, receiver.get_key(5));
//	EXPECT_EQ(17, receiver.get_key(6));
//	EXPECT_EQ(18, receiver.get_key(7));
//	EXPECT_EQ(19, receiver.get_key(8));
//
//	EXPECT_EQ(10, node.num_keys());
//	EXPECT_EQ(1, node.get_key(0));
//	EXPECT_EQ(2, node.get_key(1));
//	EXPECT_EQ(3, node.get_key(2));
//	EXPECT_EQ(4, node.get_key(3));
//	EXPECT_EQ(5, node.get_key(4));
//	EXPECT_EQ(6, node.get_key(5));
//	EXPECT_EQ(7, node.get_key(6));
//	EXPECT_EQ(8, node.get_key(7));
//	EXPECT_EQ(9, node.get_key(8));
//	EXPECT_EQ(10, node.get_key(9));
//}
//
//TEST(BTREE_NODE, TRANSFER_LARGEST_CHILDREN)
//{
//	auto node = BTreeNode(10);
//	node.insert_child(new BTreeNode(1));
//	node.insert_child(new BTreeNode(2));
//	node.insert_child(new BTreeNode(3));
//	node.insert_child(new BTreeNode(4));
//	node.insert_child(new BTreeNode(5));
//	node.insert_child(new BTreeNode(6));
//	node.insert_child(new BTreeNode(7));
//	node.insert_child(new BTreeNode(8));
//	node.insert_child(new BTreeNode(9));
//	node.insert_child(new BTreeNode(10));
//	node.insert_child(new BTreeNode(11));
//	node.insert_child(new BTreeNode(12));
//	node.insert_child(new BTreeNode(13));
//	node.insert_child(new BTreeNode(14));
//	node.insert_child(new BTreeNode(15));
//	node.insert_child(new BTreeNode(16));
//	node.insert_child(new BTreeNode(17));
//	node.insert_child(new BTreeNode(18));
//	node.insert_child(new BTreeNode(19));
//	node.insert_child(new BTreeNode(20));
//
//	auto receiver = BTreeNode(10);
//	node.transfer_largest_children(receiver);
//
//	EXPECT_EQ(10, receiver.num_children());
//	EXPECT_EQ(11, receiver.get_child(0).min_num_children());
//	EXPECT_EQ(12, receiver.get_child(1).min_num_children());
//	EXPECT_EQ(13, receiver.get_child(2).min_num_children());
//	EXPECT_EQ(14, receiver.get_child(3).min_num_children());
//	EXPECT_EQ(15, receiver.get_child(4).min_num_children());
//	EXPECT_EQ(16, receiver.get_child(5).min_num_children());
//	EXPECT_EQ(17, receiver.get_child(6).min_num_children());
//	EXPECT_EQ(18, receiver.get_child(7).min_num_children());
//	EXPECT_EQ(19, receiver.get_child(8).min_num_children());
//	EXPECT_EQ(20, receiver.get_child(9).min_num_children());
//
//	EXPECT_EQ(10, node.num_children());
//	EXPECT_EQ(1, node.get_child(0).min_num_children());
//	EXPECT_EQ(2, node.get_child(1).min_num_children());
//	EXPECT_EQ(3, node.get_child(2).min_num_children());
//	EXPECT_EQ(4, node.get_child(3).min_num_children());
//	EXPECT_EQ(5, node.get_child(4).min_num_children());
//	EXPECT_EQ(6, node.get_child(5).min_num_children());
//	EXPECT_EQ(7, node.get_child(6).min_num_children());
//	EXPECT_EQ(8, node.get_child(7).min_num_children());
//	EXPECT_EQ(9, node.get_child(8).min_num_children());
//	EXPECT_EQ(10, node.get_child(9).min_num_children());
//}
//
//TEST(BTREE_NODE, REMOVE_KEY)
//{
//	auto node = BTreeNode(10);
//	node.insert_key(1);
//	node.insert_key(2);
//	node.insert_key(3);
//	node.insert_key(4);
//	node.insert_key(5);
//
//	EXPECT_EQ(5, node.num_keys());
//
//	node.remove_key(0);
//	EXPECT_EQ(4, node.num_keys());
//	EXPECT_EQ(2, node.get_key(0));
//	EXPECT_EQ(3, node.get_key(1));
//	EXPECT_EQ(4, node.get_key(2));
//	EXPECT_EQ(5, node.get_key(3));
//
//	node.remove_key(3);
//	EXPECT_EQ(3, node.num_keys());
//	EXPECT_EQ(2, node.get_key(0));
//	EXPECT_EQ(3, node.get_key(1));
//	EXPECT_EQ(4, node.get_key(2));
//
//	node.insert_key(5);
//	node.remove_key(2);
//	EXPECT_EQ(3, node.num_keys());
//	EXPECT_EQ(2, node.get_key(0));
//	EXPECT_EQ(3, node.get_key(1));
//	EXPECT_EQ(5, node.get_key(2));
//}
//
//TEST(BTREE_NODE, INSERT_KEY_AT_INDEX)
//{
//	auto node = BTreeNode(10);
//	node.insert_key(1);
//	node.insert_key(2);
//	node.insert_key(4);
//	node.insert_key(5);
//
//	EXPECT_EQ(4, node.num_keys());
//
//	node.insert_key(3, 2);
//	EXPECT_EQ(5, node.num_keys());
//	EXPECT_EQ(1, node.get_key(0));
//	EXPECT_EQ(2, node.get_key(1));
//	EXPECT_EQ(3, node.get_key(2));
//	EXPECT_EQ(4, node.get_key(3));
//	EXPECT_EQ(5, node.get_key(4));
//
//	node.insert_key(0, 0);
//	EXPECT_EQ(6, node.num_keys());
//	EXPECT_EQ(0, node.get_key(0));
//	EXPECT_EQ(1, node.get_key(1));
//	EXPECT_EQ(2, node.get_key(2));
//	EXPECT_EQ(3, node.get_key(3));
//	EXPECT_EQ(4, node.get_key(4));
//	EXPECT_EQ(5, node.get_key(5));
//
//	node.insert_key(6, 6);
//	EXPECT_EQ(7, node.num_keys());
//	EXPECT_EQ(0, node.get_key(0));
//	EXPECT_EQ(1, node.get_key(1));
//	EXPECT_EQ(2, node.get_key(2));
//	EXPECT_EQ(3, node.get_key(3));
//	EXPECT_EQ(4, node.get_key(4));
//	EXPECT_EQ(5, node.get_key(5));
//	EXPECT_EQ(6, node.get_key(6));
//}
//
//TEST(BTREE_NODE, INSERT_KEY)
//{
//	auto node = BTreeNode(10);
//	node.insert_key(1);
//	node.insert_key(2);
//	node.insert_key(4);
//	node.insert_key(5);
//
//	EXPECT_EQ(4, node.num_keys());
//
//	node.insert_key(3);
//	EXPECT_EQ(5, node.num_keys());
//	EXPECT_EQ(1, node.get_key(0));
//	EXPECT_EQ(2, node.get_key(1));
//	EXPECT_EQ(3, node.get_key(2));
//	EXPECT_EQ(4, node.get_key(3));
//	EXPECT_EQ(5, node.get_key(4));
//
//	node.insert_key(0);
//	EXPECT_EQ(6, node.num_keys());
//	EXPECT_EQ(0, node.get_key(0));
//	EXPECT_EQ(1, node.get_key(1));
//	EXPECT_EQ(2, node.get_key(2));
//	EXPECT_EQ(3, node.get_key(3));
//	EXPECT_EQ(4, node.get_key(4));
//	EXPECT_EQ(5, node.get_key(5));
//
//	node.insert_key(6);
//	EXPECT_EQ(7, node.num_keys());
//	EXPECT_EQ(0, node.get_key(0));
//	EXPECT_EQ(1, node.get_key(1));
//	EXPECT_EQ(2, node.get_key(2));
//	EXPECT_EQ(3, node.get_key(3));
//	EXPECT_EQ(4, node.get_key(4));
//	EXPECT_EQ(5, node.get_key(5));
//	EXPECT_EQ(6, node.get_key(6));
//}
//
//TEST(BTREE_NODE, INSERT_CHILD)
//{
//	auto node = BTreeNode(10);
//	node.insert_child(new BTreeNode(2));
//	node.insert_child(new BTreeNode(3));
//	node.insert_child(new BTreeNode(5));
//	node.insert_child(new BTreeNode(6));
//
//	EXPECT_EQ(4, node.num_children());
//
//	node.insert_child(new BTreeNode(4), 2);
//	EXPECT_EQ(5, node.num_children());
//	EXPECT_EQ(2, node.get_child(0).min_num_children());
//	EXPECT_EQ(3, node.get_child(1).min_num_children());
//	EXPECT_EQ(4, node.get_child(2).min_num_children());
//	EXPECT_EQ(5, node.get_child(3).min_num_children());
//	EXPECT_EQ(6, node.get_child(4).min_num_children());
//
//	node.insert_child(new BTreeNode(1), 0);
//	EXPECT_EQ(6, node.num_children());
//	EXPECT_EQ(1, node.get_child(0).min_num_children());
//	EXPECT_EQ(2, node.get_child(1).min_num_children());
//	EXPECT_EQ(3, node.get_child(2).min_num_children());
//	EXPECT_EQ(4, node.get_child(3).min_num_children());
//	EXPECT_EQ(5, node.get_child(4).min_num_children());
//	EXPECT_EQ(6, node.get_child(5).min_num_children());
//
//	node.insert_child(new BTreeNode(7), 6);
//	EXPECT_EQ(7, node.num_children());
//	EXPECT_EQ(1, node.get_child(0).min_num_children());
//	EXPECT_EQ(2, node.get_child(1).min_num_children());
//	EXPECT_EQ(3, node.get_child(2).min_num_children());
//	EXPECT_EQ(4, node.get_child(3).min_num_children());
//	EXPECT_EQ(5, node.get_child(4).min_num_children());
//	EXPECT_EQ(6, node.get_child(5).min_num_children());
//	EXPECT_EQ(7, node.get_child(6).min_num_children());
//}
//
//TEST(BTREE_NODE, SPLIT_CHILD)
//{
//	auto child = new BTreeNode(3);
//	child->insert_key(1);
//	child->insert_key(2);
//	child->insert_key(3);
//	child->insert_key(4);
//	child->insert_key(5);
//	child->insert_child(new BTreeNode(1));
//	child->insert_child(new BTreeNode(2));
//	child->insert_child(new BTreeNode(3));
//	child->insert_child(new BTreeNode(4));
//	child->insert_child(new BTreeNode(5));
//	child->insert_child(new BTreeNode(6));
//
//	auto parent = BTreeNode(3);
//	parent.insert_key(0);
//	parent.insert_key(6);
//	parent.insert_child(new BTreeNode(1));
//	parent.insert_child(new BTreeNode(3));
//	parent.insert_child(child, 1);
//
//	EXPECT_EQ(2, parent.num_keys());
//	EXPECT_EQ(3, parent.num_children());
//
//	parent.split_child(1);
//
//	EXPECT_EQ(3, parent.num_keys());
//	EXPECT_EQ(4, parent.num_children());
//	EXPECT_EQ(3, parent.get_key(1));
//
//	auto& new_left_child = parent.get_child(1);
//	EXPECT_EQ(3, new_left_child.num_children());
//	EXPECT_EQ(1, new_left_child.get_child(0).min_num_children());
//	EXPECT_EQ(2, new_left_child.get_child(1).min_num_children());
//	EXPECT_EQ(3, new_left_child.get_child(2).min_num_children());
//	EXPECT_EQ(2, new_left_child.num_keys());
//	EXPECT_EQ(1, new_left_child.get_key(0));
//	EXPECT_EQ(2, new_left_child.get_key(1));
//
//	auto& new_right_child = parent.get_child(2);
//	EXPECT_EQ(3, new_right_child.num_children());
//	EXPECT_EQ(4, new_right_child.get_child(0).min_num_children());
//	EXPECT_EQ(5, new_right_child.get_child(1).min_num_children());
//	EXPECT_EQ(6, new_right_child.get_child(2).min_num_children());
//	EXPECT_EQ(2, new_right_child.num_keys());
//	EXPECT_EQ(4, new_right_child.get_key(0));
//	EXPECT_EQ(5, new_right_child.get_key(1));
//}
//
//TEST(BTREE_NODE, BINARY_FIND_KEY)
//{
//	auto node = BTreeNode(6);
//	node.insert_key(1);
//	node.insert_key(2);
//	node.insert_key(3);
//	node.insert_key(4);
//	node.insert_key(5);
//	node.insert_key(6);
//	node.insert_key(7);
//	node.insert_key(8);
//	node.insert_key(9);
//	node.insert_key(10);
//
//	EXPECT_EQ(true, node.binary_find_key(1));
//	EXPECT_EQ(true, node.binary_find_key(2));
//	EXPECT_EQ(true, node.binary_find_key(3));
//	EXPECT_EQ(true, node.binary_find_key(4));
//	EXPECT_EQ(true, node.binary_find_key(5));
//	EXPECT_EQ(true, node.binary_find_key(6));
//	EXPECT_EQ(true, node.binary_find_key(7));
//	EXPECT_EQ(true, node.binary_find_key(8));
//	EXPECT_EQ(true, node.binary_find_key(9));
//	EXPECT_EQ(true, node.binary_find_key(10));
//
//	EXPECT_EQ(false, node.binary_find_key(0));
//	EXPECT_EQ(false, node.binary_find_key(11));
//	EXPECT_EQ(false, node.binary_find_key(12));
//	EXPECT_EQ(false, node.binary_find_key(13));
//	EXPECT_EQ(false, node.binary_find_key(14));
//	EXPECT_EQ(false, node.binary_find_key(15));
//	EXPECT_EQ(false, node.binary_find_key(16));
//	EXPECT_EQ(false, node.binary_find_key(17));
//	EXPECT_EQ(false, node.binary_find_key(18));
//}
//
//TEST(BTREE_NODE, LINEAR_FIND_KEY)
//{
//	auto node = BTreeNode(6);
//	node.insert_key(1);
//	node.insert_key(2);
//	node.insert_key(3);
//	node.insert_key(4);
//	node.insert_key(5);
//	node.insert_key(6);
//	node.insert_key(7);
//	node.insert_key(8);
//	node.insert_key(9);
//	node.insert_key(10);
//
//	EXPECT_EQ(true, node.linear_find_key(1));
//	EXPECT_EQ(true, node.linear_find_key(2));
//	EXPECT_EQ(true, node.linear_find_key(3));
//	EXPECT_EQ(true, node.linear_find_key(4));
//	EXPECT_EQ(true, node.linear_find_key(5));
//	EXPECT_EQ(true, node.linear_find_key(6));
//	EXPECT_EQ(true, node.linear_find_key(7));
//	EXPECT_EQ(true, node.linear_find_key(8));
//	EXPECT_EQ(true, node.linear_find_key(9));
//	EXPECT_EQ(true, node.linear_find_key(10));
//
//	EXPECT_EQ(false, node.linear_find_key(0));
//	EXPECT_EQ(false, node.linear_find_key(11));
//	EXPECT_EQ(false, node.linear_find_key(12));
//	EXPECT_EQ(false, node.linear_find_key(13));
//	EXPECT_EQ(false, node.linear_find_key(14));
//	EXPECT_EQ(false, node.linear_find_key(15));
//	EXPECT_EQ(false, node.linear_find_key(16));
//	EXPECT_EQ(false, node.linear_find_key(17));
//	EXPECT_EQ(false, node.linear_find_key(18));
//}
//
//TEST(BTREE_NODE, LINEAR_FIND_KEY_RECURSIVE)
//{
//	auto tree = fill_tree(10, 500);
//
//	EXPECT_EQ(true, tree->linear_find_key(1));
//	EXPECT_EQ(true, tree->linear_find_key(2));
//	EXPECT_EQ(true, tree->linear_find_key(3));
//	EXPECT_EQ(true, tree->linear_find_key(4));
//	EXPECT_EQ(true, tree->linear_find_key(5));
//	EXPECT_EQ(true, tree->linear_find_key(6));
//	EXPECT_EQ(true, tree->linear_find_key(7));
//	EXPECT_EQ(true, tree->linear_find_key(8));
//	EXPECT_EQ(true, tree->linear_find_key(9));
//	EXPECT_EQ(true, tree->linear_find_key(99));
//	EXPECT_EQ(true, tree->linear_find_key(68));
//	EXPECT_EQ(true, tree->linear_find_key(50));
//	EXPECT_EQ(true, tree->linear_find_key(111));
//	EXPECT_EQ(true, tree->linear_find_key(222));
//	EXPECT_EQ(true, tree->linear_find_key(333));
//	EXPECT_EQ(true, tree->linear_find_key(444));
//	EXPECT_EQ(true, tree->linear_find_key(500));
//
//	EXPECT_EQ(false, tree->linear_find_key(-1));
//	EXPECT_EQ(false, tree->linear_find_key(0));
//	EXPECT_EQ(false, tree->linear_find_key(511));
//	EXPECT_EQ(false, tree->linear_find_key(512));
//	EXPECT_EQ(false, tree->linear_find_key(513));
//	EXPECT_EQ(false, tree->linear_find_key(514));
//	EXPECT_EQ(false, tree->linear_find_key(815));
//	EXPECT_EQ(false, tree->linear_find_key(516));
//	EXPECT_EQ(false, tree->linear_find_key(717));
//	EXPECT_EQ(false, tree->linear_find_key(518));
//	EXPECT_EQ(false, tree->linear_find_key(2000));
//}
//
//TEST(BTREE_5, INSERT_50)
//{
//	auto result = validate_insert_keys(5, 50);
//	EXPECT_EQ(true, result.valid) << result.message;
//}
//
//TEST(BTREE_5, INSERT_500)
//{
//	auto result = validate_insert_keys(5, 500);
//	EXPECT_EQ(true, result.valid) << result.message;
//}
//
//TEST(BTREE_5, INSERT_1000)
//{
//	auto result = validate_insert_keys(5, 1000);
//	EXPECT_EQ(true, result.valid) << result.message;
//}
//
//TEST(BTREE_50, INSERT_50)
//{
//	auto result = validate_insert_keys(50, 50);
//	EXPECT_EQ(true, result.valid) << result.message;
//}
//
//TEST(BTREE_50, INSERT_500)
//{
//	auto result = validate_insert_keys(50, 500);
//	EXPECT_EQ(true, result.valid) << result.message;
//}
//
//TEST(BTREE_50, INSERT_1000)
//{
//	auto result = validate_insert_keys(50, 1000);
//	EXPECT_EQ(true, result.valid) << result.message;
//}
//
//TEST(BTREE_1000, INSERT_1000)
//{
//	auto result = validate_insert_keys(1000, 1000);
//	EXPECT_EQ(true, result.valid) << result.message;
//}
//
//TEST(BTREE_1000, INSERT_5000)
//{
//	auto result = validate_insert_keys(1000, 5000);
//	EXPECT_EQ(true, result.valid) << result.message;
//}