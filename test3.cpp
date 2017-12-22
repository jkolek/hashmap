#include <mutex>
#include <thread>
#include <iostream>
#include <cassert>

#include "hashmap.h"

constexpr unsigned MAX_TABLE_SIZE = 100;
static constexpr unsigned HASH_CONST = 17;  /* A prime number */

class IntHash
{
public:
    unsigned operator()(int key)
    {
        return (key * key + HASH_CONST) % MAX_TABLE_SIZE;
    }
};

class StringHash
{
public:
    unsigned operator()(std::string key)
    {
        unsigned res = 0;
        std::string::iterator it = key.begin();

        while (it != key.end())
        {
            res += (unsigned) *it + HASH_CONST;
            ++it;
        }

        return res;
    }
};

std::mutex mtx;
HashMap<unsigned, std::string, IntHash> imap(MAX_TABLE_SIZE);

int main()
{
    const unsigned n1 = 10;
    const unsigned n2 = 20;
    const unsigned n3 = 33;
    const unsigned n4 = 234;
    const unsigned n5 = 243;
    const unsigned n6 = 254;

    for (unsigned i = 0; i < 10; i++)
    {
        imap.insert(n1 + i, "pineapple");
        imap.insert(n2 + i, "mango");
        imap.insert(n3 + i, "apple");
        imap.insert(n4 + i, "orange");
        imap.insert(n5 + i, "banana");
        imap.insert(n6 + i, "kiwi");
    }

    // Test iterator

    HashMap<unsigned, std::string, IntHash>::Iterator imIter = imap.begin();

    while (imIter != imap.end())
    {
        std::cout << (*imIter)->value << std::endl;
        ++imIter;
    }

    std::cout << "===================================" << std::endl;

    imIter = imap.begin();

    while (imIter != imap.end())
    {
        HashMap<unsigned, std::string, IntHash>::Element *e = (*imIter);
        std::cout << "key   == " << e->key << std::endl;
        std::cout << "value == " << e->value << std::endl;
        imIter++;
    }

    // Test move constructor

    HashMap<unsigned, std::string, IntHash> imap2 = std::move(imap);

    std::cout << "imap.getSize()  == " << imap.getSize() << std::endl;
    std::cout << "imap2.getSize() == " << imap2.getSize() << std::endl;

    // Test move assignment operator

    HashMap<unsigned, std::string, IntHash> imap3;

    imap3 = std::move(imap2);

    std::cout << "imap2.getSize() == " << imap2.getSize() << std::endl;
    std::cout << "imap3.getSize() == " << imap3.getSize() << std::endl;
}
