#include <iostream>
#include <cassert>

#include "hashmap.h"

constexpr unsigned MAX_TABLE_SIZE = 100;
static constexpr unsigned hashConst = 17; // A prime number

class UnsignedHash
{
public:
    unsigned operator()(unsigned key)
    {
        return (key * key + hashConst) % MAX_TABLE_SIZE;
    }
};

HashMap<unsigned, std::string, UnsignedHash> umap(MAX_TABLE_SIZE);

int main()
{
    std::string msg;

    umap.insert(25, "hello");
    umap.insert(34, "world");
    umap.insert(43, "one");
    umap.insert(143, "two");
    umap.insert(754, "three");

    assert(umap.lookup(25) == "hello");
    assert(umap.lookup(34) == "world");
    assert(umap.lookup(43) == "one");
    assert(umap.lookup(143) == "two");
    assert(umap.lookup(754) == "three");

    assert(umap.exists(25) == true);
    assert(umap.exists(34) == true);
    assert(umap.exists(43) == true);
    assert(umap.exists(143) == true);
    assert(umap.exists(754) == true);

    umap.remove(25);
    umap.remove(143);

    assert(umap.exists(25) == false);
    assert(umap.exists(143) == false);

    umap.insert(43, "new value");
    assert(umap.lookup(43) == "new value");

    umap.insert(143, "143");
    assert(umap.lookup(143) == "143");

    try
    {
        // Try to lookup non-existing key
        umap.lookup(30);
    }
    catch (std::out_of_range &e)
    {
        msg = e.what();
    }

    assert(msg == "HashMap: key doesn't exists");

    try
    {
        // Try to remove non-existing key
        umap.remove(60);
    }
    catch (std::out_of_range &e)
    {
        msg = e.what();
    }

    assert(msg == "HashMap: key doesn't exists");

    // Test move constructor

    size_t tmpSize = umap.getSize();
    HashMap<unsigned, std::string, UnsignedHash> umap2 = std::move(umap);

    assert(umap.getSize() == 0);
    assert(umap2.getSize() == tmpSize);

    // Test move assignment operator

    tmpSize = umap2.getSize();
    HashMap<unsigned, std::string, UnsignedHash> umap3;
    umap3 = std::move(umap2);

    assert(umap2.getSize() == 0);
    assert(umap3.getSize() == tmpSize);

    // Test copy constructor

    tmpSize = umap3.getSize();
    HashMap<unsigned, std::string, UnsignedHash> umap4 = umap3;

    assert(umap3.getSize() == tmpSize);
    assert(umap4.getSize() == tmpSize);
    assert(umap3.lookup(43) == umap4.lookup(43));

    // Test copy assignment operator

    tmpSize = umap4.getSize();
    HashMap<unsigned, std::string, UnsignedHash> umap5;
    umap5 = umap4;

    assert(umap4.getSize() == tmpSize);
    assert(umap5.getSize() == tmpSize);
    assert(umap4.lookup(754) == umap5.lookup(754));

    std::cout << "Success!" << std::endl;
}
