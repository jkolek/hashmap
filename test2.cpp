#include <mutex>
#include <thread>
#include <iostream>

#include "hashmap.h"

constexpr unsigned MAX_TABLE_SIZE = 100;
static constexpr unsigned HASH_CONST = 17;  // A prime number

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

constexpr unsigned NEW_MAX_TABLE_SIZE = MAX_TABLE_SIZE * 2;

std::mutex mtx;
HashMap<unsigned, std::string, IntHash> im(MAX_TABLE_SIZE);
HashMap<std::string, std::string, StringHash> sm(MAX_TABLE_SIZE);

void insert1(unsigned id)
{
    const unsigned n1 = 10+id;
    const unsigned n2 = 20+id;
    const unsigned n3 = 33+id;
    const unsigned n4 = 234+id;
    const unsigned n5 = 243+id;
    const unsigned n6 = 254+id;

    for (unsigned i = 0; i < 10; ++i)
    {
        im.insert(n1 + i, "pineapple");
        im.insert(n2 + i, "mango");
        im.insert(n3 + i, "apple");
        im.insert(n4 + i, "orange");
        im.insert(n5 + i, "banana");
        im.insert(n6 + i, "kiwi");

        {
            std::lock_guard<std::mutex> lock(mtx);
            std::string val;

            try
            {
                val = im.lookup(n5 + i);
            }
            catch (std::out_of_range &e)
            {
                val = e.what();
            }

            std::cout << id << " : " << val << std::endl;

            try
            {
                val = im.lookup(n3 + i);
            }
            catch (std::out_of_range &e)
            {
                val = e.what();
            }

            std::cout << id << " : " << val << std::endl;

            try
            {
                im.remove(n5 + i);
            }
            catch (std::out_of_range &e)
            {
                val = e.what();
            }

            std::cout << id << " : " << val << std::endl;

            try
            {
                im.remove(n2 + i);
            }
            catch (std::out_of_range &e)
            {
                val = e.what();
            }

            std::cout << id << " : " << val << std::endl;

            try
            {
                im.remove(n3 + i);
            }
            catch (std::out_of_range &e)
            {
                val = e.what();
            }

            std::cout << id << " : " << val << std::endl;
        }
    }
}

void insert2(unsigned id)
{
    std::string key1 = "pineapple" + std::to_string(id);
    std::string key2 = "mango" + std::to_string(id);
    std::string key3 = "apple" + std::to_string(id);
    std::string key4 = "orange" + std::to_string(id);
    std::string key5 = "banana" + std::to_string(id);
    std::string key6 = "kiwi" + std::to_string(id);

    for (unsigned i = 0; i < 10; ++i)
    {
        std::string key1i = key1 + std::to_string(i);
        std::string key2i = key2 + std::to_string(i);
        std::string key3i = key3 + std::to_string(i);
        std::string key4i = key4 + std::to_string(i);
        std::string key5i = key5 + std::to_string(i);
        std::string key6i = key6 + std::to_string(i);

        sm.insert(key1i, "pineapple");
        sm.insert(key2i, "mango");
        sm.insert(key3i, "apple");
        sm.insert(key4i, "orange");
        sm.insert(key5i, "banana");
        sm.insert(key6i, "kiwi");

        {
            std::lock_guard<std::mutex> lock(mtx);
            std::string val;

            try
            {
                val = sm.lookup(key1i);
            }
            catch (std::out_of_range &e)
            {
                val = e.what();
            }

            std::cout << id << " : " << val << std::endl;

            try
            {
                val = sm.lookup(key5i);
            }
            catch (std::out_of_range &e)
            {
                val = e.what();
            }

            std::cout << id << " : " << val << std::endl;

            try
            {
                sm.remove(key5i);
            }
            catch (std::out_of_range &e)
            {
                val = e.what();
            }

            std::cout << id << " : " << val << std::endl;

            try
            {
                sm.remove(key2i);
            }
            catch (std::out_of_range &e)
            {
                val = e.what();
            }

            std::cout << id << " : " << val << std::endl;

            try
            {
                sm.remove(key3i);
            }
            catch (std::out_of_range &e)
            {
                val = e.what();
            }

            std::cout << id << " : " << val << std::endl;
        }
    }
}

int main()
{
    std::thread t1(std::bind(&insert1, 1));
    std::thread t2(std::bind(&insert1, 2));
    std::thread t3(std::bind(&insert1, 3));

    //im.resize(MAX_TABLE_SIZE * 2);

    std::thread t4(std::bind(&insert1, 4));
    std::thread t5(std::bind(&insert1, 5));

    std::thread t6(std::bind(&insert2, 1));
    std::thread t7(std::bind(&insert2, 2));
    std::thread t8(std::bind(&insert2, 3));

    //sm.resize(NEW_MAX_TABLE_SIZE);

    std::thread t9(std::bind(&insert2, 4));
    std::thread t10(std::bind(&insert2, 5));

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();

    t6.join();
    t7.join();
    t8.join();
    t9.join();
    t10.join();

    sm.print();

    std::cout << std::endl;
    im.print();
}
