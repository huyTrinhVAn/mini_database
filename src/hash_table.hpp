#pragma once
#include <string>
#include <utility>
#include <vector>

class HashTable
{
public:
    HashTable();

    void put(const std::string &key, const std::string &value);
    bool get(const std::string &key, std::string &out_value);
    bool contains(const std::string &key);
    bool erase(const std::string &key);

private:
    std::vector<std::vector<std::pair<std::string, std::string>>> buckets_;
};
HashTable::HashTable()
{
    buckets_.resize(16);
};

void HashTable::put(const std::string &key, const std::string &value)
{
    // std :: hash<std::string> hasher;
    // std::size_t hash_value = hasher(key);
    std::size_t hash_value = std::hash<std::string>{}(key);
    size_t index = hash_value % buckets_.size();
    auto &bucket = buckets_[index];
    for (auto &p : bucket)
    {
        if (p.first == key)
        {
            p.second = value;
            return;
        }
    }
    // bucket.push_back({key, value});
    bucket.emplace_back(key, value);
}
bool HashTable::get(const std::string &key, std::string &out_value)
{
    std::size_t hash_value = std::hash<std::string>{}(key);
    size_t index = hash_value % buckets_.size();
    auto &bucket = buckets_[index];
    for (auto &p : bucket)
    {

        if (p.first == key)
        {
            out_value = p.second;
            return true;
        }
    }
    return false;
}

// bool HashTable :: contains(const std::string &key){
//     std::size_t hash_value = std::hash<std::string>{}(key);
//     size_t index = hash_value % buckets_.size();
//     auto &bucket = buckets_[index];
//     for (auto &p : bucket)
//     {

//         if (p.first == key)
//         {
//             return true;
//         }
//     }
//     return false;
// }

// follow DRY principle
bool HashTable::contains(const std::string &key)
{
    std::string dummy;
    return get(key, dummy);
}
bool HashTable::erase(const std::string &key)
{
    std::size_t hash_value = std::hash<std::string>{}(key);
    size_t index = hash_value % buckets_.size();
    auto &bucket = buckets_[index];
    for (auto it = bucket.begin(); it != bucket.end(); ++it)
    {
        if (it->first == key)
        {
            bucket.erase(it);
            return true;
        }
    }
    return false;
}