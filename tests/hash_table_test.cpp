#include <gtest/gtest.h>

#include "hash_table.hpp"

TEST(HashTableTest, PutThenGetReturnsValue)
{
    HashTable table;
    table.put("a", "1");
    table.put("b", "2");

    std::string value;
    EXPECT_TRUE(table.get("a", value));
    EXPECT_EQ(value, "1");
    EXPECT_TRUE(table.get("b", value));
    EXPECT_EQ(value, "2");
}

TEST(HashTableTest, GetMissingKeyReturnsFalse)
{
    HashTable table;
    std::string value;
    EXPECT_FALSE(table.get("missing", value));
}

TEST(HashTableTest, PutOverwritesExistingKey)
{
    HashTable table;
    table.put("a", "1");
    table.put("a", "2");

    std::string value;
    EXPECT_TRUE(table.get("a", value));
    EXPECT_EQ(value, "2");
}

TEST(HashTableTest, ContainsReflectsPresence)
{
    HashTable table;
    EXPECT_FALSE(table.contains("a"));
    table.put("a", "1");
    EXPECT_TRUE(table.contains("a"));
}

TEST(HashTableTest, EraseRemovesKey)
{
    HashTable table;
    table.put("a", "1");
    EXPECT_TRUE(table.erase("a"));
    EXPECT_FALSE(table.contains("a"));
}

TEST(HashTableTest, EraseMissingKeyReturnsFalse)
{
    HashTable table;
    EXPECT_FALSE(table.erase("missing"));
}

TEST(HashTableTest, RehashPreservesAllElements)
{
    HashTable table;
    const int kCount = 1000;

    for (int i = 0; i < kCount; ++i)
    {
        table.put("key" + std::to_string(i), "val" + std::to_string(i));
    }

    EXPECT_EQ(table.size(), static_cast<std::size_t>(kCount));
    EXPECT_LE(table.load_factor(), 0.75);

    std::string value;
    for (int i = 0; i < kCount; ++i)
    {
        ASSERT_TRUE(table.get("key" + std::to_string(i), value))
            << "missing key" << i << " after rehash";
        EXPECT_EQ(value, "val" + std::to_string(i));
    }
}
