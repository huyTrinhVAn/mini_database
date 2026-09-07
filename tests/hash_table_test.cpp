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

// NOTE: no rehash/resize test yet — HashTable doesn't implement automatic
// resizing on load factor yet (see docs/01-hash-table.md). Add a test here
// once size()/load_factor()/rehash are implemented.
