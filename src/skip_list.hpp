#pragma once
#include <string>
#include <utility>
#include <vector>

struct SkipListNode
{
    std::string key;
    std::string value;
    std::vector<SkipListNode *> forward;

    SkipListNode(const std::string &key, const std::string &value, int level);
};

class SkipList
{
public:
    SkipList();
    ~SkipList();

    void put(const std::string &key, const std::string &value);
    bool get(const std::string &key, std::string &out_value);
    bool contains(const std::string &key);
    bool erase(const std::string &key);
    std::vector<std::pair<std::string, std::string>> range(const std::string &start, const std::string &end);

private:
    static const int kMaxLevel = 16;

    SkipListNode *head_;
    int level_;

    int random_level();
};
