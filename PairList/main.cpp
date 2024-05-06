#include <iostream>
#include "CuPairList.h"

int main()
{
    CU::PairList<int, std::string> list{};

    std::cout << list[0] << std::endl;
    // std::cout << list("first-val") << std::endl; Only support add key.

    list.add(1, "Hello");
    list.add(2, "World");
    list.add(3, "Pair");

    std::cout << "front: " << list.front().first << " " << list.front().second << std::endl;
    std::cout << "back: " << list.back().first << " " << list.back().second << std::endl;
    for (const auto &[key, value] : list) {
        std::cout << key << " " << value << std::endl;
    }
    std::cout << "size: " << list.end() - list.begin() << std::endl;

    list[1] = "GoodBye";
    list("Pair") = 6;

    std::cout << list[1] << std::endl;
    std::cout << list("Pair") << std::endl;
    for (auto iter = list.begin(); iter < list.end(); iter++) {
        std::cout << iter.key() << " " << iter.value() << std::endl;
    }
}
