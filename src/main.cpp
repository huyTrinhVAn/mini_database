#include <iostream>

#include "hash_table.hpp"

int main()
{
    HashTable table;

    table.put("name", "alice");
    table.put("role", "engineer");
    table.put("name", "bob"); // overwrite key "name"

    std::string result;
    if (table.get("name", result))
    {
        std::cout << "name = " << result << "\n";
    }
    else
    {
        std::cout << "name not found\n";
    }

    if (table.get("role", result))
    {
        std::cout << "role = " << result << "\n";
    }
    else
    {
        std::cout << "role not found\n";
    }

    if (table.get("missing_key", result))
    {
        std::cout << "missing_key = " << result << "\n";
    }
    else
    {
        std::cout << "missing_key not found\n";
    }

    std::cout << "contains(role) before erase = " << table.contains("role") << "\n";
    std::cout << "erase(role) = " << table.erase("role") << "\n";
    std::cout << "contains(role) after erase = " << table.contains("role") << "\n";
    std::cout << "erase(role) again = " << table.erase("role") << "\n";

    return 0;
}
