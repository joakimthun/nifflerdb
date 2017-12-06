#include <gtest\gtest.h>
#include <vector>

#include "bp_tree.h"

using namespace niffler;

TEST(KEY_COMP, KEY_COMP)
{
    key k0(123);
    key k1(123);

    EXPECT_TRUE(k0 == k1);
    EXPECT_FALSE(k0 != k1);
    EXPECT_FALSE(k0 < k1);
    EXPECT_FALSE(k0 > k1);
    EXPECT_TRUE(k0 <= k1);
    EXPECT_TRUE(k0 >= k1);

    k0 = "123";
    k1 = "123";
    
    EXPECT_TRUE(k0 == k1);
    EXPECT_FALSE(k0 != k1);
    EXPECT_FALSE(k0 < k1);
    EXPECT_FALSE(k0 > k1);
    EXPECT_TRUE(k0 <= k1);
    EXPECT_TRUE(k0 >= k1);

    k0 = "abc";
    k1 = "def";

    EXPECT_FALSE(k0 == k1);
    EXPECT_TRUE(k0 != k1);
    EXPECT_TRUE(k0 < k1);
    EXPECT_FALSE(k0 > k1);
    EXPECT_TRUE(k0 <= k1);
    EXPECT_FALSE(k0 >= k1);

    k0 = "";
    k1 = "";

    EXPECT_TRUE(k0 == k1);
    EXPECT_FALSE(k0 != k1);
    EXPECT_FALSE(k0 < k1);
    EXPECT_FALSE(k0 > k1);
    EXPECT_TRUE(k0 <= k1);
    EXPECT_TRUE(k0 >= k1);

    k0 = "";
    k1 = "1";

    EXPECT_FALSE(k0 == k1);
    EXPECT_TRUE(k0 != k1);
    EXPECT_TRUE(k0 < k1);
    EXPECT_FALSE(k0 > k1);
    EXPECT_TRUE(k0 <= k1);
    EXPECT_FALSE(k0 >= k1);

    k0 = 0;
    k1 = 12;

    EXPECT_FALSE(k0 == k1);
    EXPECT_TRUE(k0 != k1);
    EXPECT_TRUE(k0 < k1);
    EXPECT_FALSE(k0 > k1);
    EXPECT_TRUE(k0 <= k1);
    EXPECT_FALSE(k0 >= k1);

    k0 = "abc";
    k1 = "abcd";

    EXPECT_FALSE(k0 == k1);
    EXPECT_TRUE(k0 != k1);
    EXPECT_TRUE(k0 < k1);
    EXPECT_FALSE(k0 > k1);
    EXPECT_TRUE(k0 <= k1);
    EXPECT_FALSE(k0 >= k1);
}