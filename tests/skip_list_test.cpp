#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <string>
#include <vector>

#include "skip_list.hpp"

TEST(SkipListTest, PutThenGetReturnsValue)
{
    SkipList list;
    list.put("a", "1");
    list.put("b", "2");

    std::string value;
    EXPECT_TRUE(list.get("a", value));
    EXPECT_EQ(value, "1");
    EXPECT_TRUE(list.get("b", value));
    EXPECT_EQ(value, "2");
}

TEST(SkipListTest, GetMissingKeyReturnsFalse)
{
    SkipList list;
    std::string value;
    EXPECT_FALSE(list.get("missing", value));
}

TEST(SkipListTest, GetOnEmptyListReturnsFalse)
{
    SkipList list;
    std::string value;
    EXPECT_FALSE(list.get("anything", value));
}

TEST(SkipListTest, PutOverwritesExistingKey)
{
    SkipList list;
    list.put("a", "1");
    list.put("a", "2");

    std::string value;
    EXPECT_TRUE(list.get("a", value));
    EXPECT_EQ(value, "2");
}

TEST(SkipListTest, ManyKeysInsertedOutOfOrderAreAllRetrievable)
{
    SkipList list;
    std::vector<int> keys;
    for (int i = 0; i < 500; ++i)
    {
        keys.push_back(i);
    }
    std::mt19937 rng(42);
    std::shuffle(keys.begin(), keys.end(), rng);

    for (int k : keys)
    {
        std::string key = "key" + std::to_string(k);
        list.put(key, "val" + std::to_string(k));
    }

    std::string value;
    for (int k = 0; k < 500; ++k)
    {
        std::string key = "key" + std::to_string(k);
        ASSERT_TRUE(list.get(key, value)) << "missing " << key;
        EXPECT_EQ(value, "val" + std::to_string(k));
    }
}
