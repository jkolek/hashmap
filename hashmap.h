// The MIT License (MIT)
//
// Thread-safe generic hashmap
// Copyright (c) 2016, 2017 Jozef Kolek <jkolek@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdexcept>
#include <mutex>

template <class K, class V, class F>
class HashMap
{
public:
    struct Element
    {
        K key;
        V value;
        Element *next = nullptr;

        Element(K k, V v) : key(k), value(v) {}
    };

private:
    // Table size
    size_t _size;

    //
    // The table elements are pointers to Element. Table contains linked
    // lists where elements of the list represents entries with key-value pairs.
    //
    Element **_table;

    //
    // We have one mutex for each _index of table, so multiple indexes can be
    // accessed at the same time.
    //
    std::mutex **_mutexes;

    //
    // Hash function is actually a class used as functor. This function
    // calculates an _index where an element needs to be stored. The calculated
    // _index is modulo of MAX_TABLE_SIZE.
    //
    F hashFunctor;

    unsigned hash(K key)
    {
        return hashFunctor(key) % _size;
    }

    void allocateTableAndMutexes(size_t size);
    void destroyTableAndMutexes();

public:
    ~HashMap();
    bool exists(K key);
    V lookup(K key);
    void insert(K key, V value);
    void remove(K key);
    void resize(size_t newSize);
    void print();
    size_t getSize() { return _size; }
    Element **getTable() { return _table; }

    HashMap() : _size(0), _table(nullptr), _mutexes(nullptr) {}
    HashMap(size_t Size);
    HashMap(HashMap &other);             // Copy constructor
    HashMap(HashMap &&other);            // Move constructor

    HashMap& operator=(HashMap &other);  // Copy assignment operator
    HashMap& operator=(HashMap &&other); // Move assignment operator

    // Indexed access of HashMap elements
    // T & operator[](int n) { return _data[n]; }
    V operator[](K key) const { return lookup(key); }

    //
    // Iterator class
    //
    class Iterator
    {
        HashMap<K, V, F> *_map;
        Element *_current = nullptr;
        unsigned _index = 0;

        void next()
        {
            if (_current == nullptr)
                return;

            if (_current->next == nullptr)
            {
                size_t size = _map->getSize();

                // Find a next element in the table different then nullptr
                do
                {
                    ++_index;
                }
                while (_index < size && _map->getTable()[_index] == nullptr);

                if (_index < size)
                    _current = _map->getTable()[_index];
                else
                    _current = nullptr;
            }
            else
            {
                _current = _current->next;
            }
        }

    public:
        Iterator() {}
        Iterator(HashMap<K, V, F> *map, Element *current)
            : _map(map), _current(current) {}

        // Prefix increment operator
        Iterator & operator++()
        {
            next();
            return *this;
        }

        // Postfix increment operator
        Iterator operator++(int)
        {
            Iterator tmp = *this;
            next();
            return tmp;
        }

        Element * operator*()
        {
            return _current;
        }

        bool operator==(Iterator other)
        {
            return other._current == _current;
        }

        bool operator!=(Iterator other)
        {
            return other._current != _current;
        }
    };

    Iterator begin()
    {
        Element *first = nullptr;
        unsigned i = 0;
        while (i < _size && _table[i] == nullptr)
            ++i;
        if (i < _size)
            first = _table[i];
        return Iterator(this, first);
    }

    Iterator end() { return Iterator(this, nullptr); }
};

//====----------------------------------------------------------------------====
// Implementation of the HashMap methods
//====----------------------------------------------------------------------====

//
// Sets _size to size, and allocates new table and mutexes
//
template <class K, class V, class F>
void HashMap<K, V, F>::allocateTableAndMutexes(size_t size)
{
    _size = size;
    _table = new Element *[_size];
    _mutexes = new std::mutex *[_size];

    for (unsigned i = 0; i < _size; ++i)
        _mutexes[i] = new std::mutex;

    for (unsigned i = 0; i < _size; ++i)
    {
        // Lock access to table elements at i.
        std::lock_guard<std::mutex> lock(*_mutexes[i]);

        _table[i] = nullptr;
    }
}

//
// Deallocates table and mutexes
//
template <class K, class V, class F>
void HashMap<K, V, F>::destroyTableAndMutexes()
{
    if (_table == nullptr)
        return;

    for (unsigned i = 0; i < _size; ++i)
    {
        {
            // Lock access to table elements at i.
            std::lock_guard<std::mutex> lock(*_mutexes[i]);

            if (_table[i] == nullptr)
                continue;

            Element *tmp = _table[i];

            while (tmp != nullptr)
            {
                Element *old = tmp;
                tmp = tmp->next;
                delete old;
            }
        }
        delete _mutexes[i];
    }

    delete [] _table;
    delete [] _mutexes;
    _size = 0;
}

template <class K, class V, class F>
HashMap<K, V, F>::HashMap(size_t size)
{
    allocateTableAndMutexes(size);
}

//
// Copy constructor
//
template <class K, class V, class F>
HashMap<K, V, F>::HashMap(HashMap &other)
{
    allocateTableAndMutexes(other._size);

    // Insert elements from other
    for (unsigned i = 0; i < other._size; ++i)
    {
        // Lock access to table elements at i.
        std::lock_guard<std::mutex> lock(*other._mutexes[i]);

        if (other._table[i] == nullptr)
            continue;

        Element *tmp = other._table[i];

        while (tmp != nullptr)
        {
            insert(tmp->key, tmp->value);
            tmp = tmp->next;
        }
    }
}

//
// Move constructor
//
template <class K, class V, class F>
HashMap<K, V, F>::HashMap(HashMap &&other)
{
    _table = other._table;
    _mutexes = other._mutexes;
    _size = other._size;

    other._table = nullptr;
    other._mutexes = nullptr;
    other._size = 0;
}

//
// Copy assignment operator
//
template <class K, class V, class F>
HashMap<K, V, F>& HashMap<K, V, F>::operator=(HashMap &other)
{
    if (this != &other)
    {
        destroyTableAndMutexes();
        allocateTableAndMutexes(other._size);

        // Insert elements from other
        for (unsigned i = 0; i < other._size; ++i)
        {
            // Lock access to table elements at i.
            std::lock_guard<std::mutex> lock(*other._mutexes[i]);

            if (other._table[i] == nullptr)
                continue;

            Element *tmp = other._table[i];

            while (tmp != nullptr)
            {
                insert(tmp->key, tmp->value);
                tmp = tmp->next;
            }
        }
    }
    return *this;
}

//
// Move assignment operator
//
template <class K, class V, class F>
HashMap<K, V, F>& HashMap<K, V, F>::operator=(HashMap &&other)
{
    if (this != &other)
    {
        destroyTableAndMutexes();

        _table = other._table;
        _mutexes = other._mutexes;
        _size = other._size;

        other._table = nullptr;
        other._mutexes = nullptr;
        other._size = 0;
    }
}

template <class K, class V, class F>
HashMap<K, V, F>::~HashMap()
{
    destroyTableAndMutexes();
}

//
// Checks if key exists.
//
template <class K, class V, class F>
bool HashMap<K, V, F>::exists(K key)
{
    unsigned i = hash(key);
    // Lock access to table elements at i.
    std::lock_guard<std::mutex> lock(*_mutexes[i]);
    Element *tmp = _table[i];

    while (tmp != nullptr && tmp->key != key)
        tmp = tmp->next;

    return tmp != nullptr;
}

//
// Returns value for given key. If key doesn't exists throws "out of range"
// exception.
//
template <class K, class V, class F>
V HashMap<K, V, F>::lookup(K key)
{
    unsigned i = hash(key);
    // Lock access to table elements at i.
    std::lock_guard<std::mutex> lock(*_mutexes[i]);
    Element *tmp = _table[i];

    while (tmp != nullptr && tmp->key != key)
        tmp = tmp->next;

    if (tmp == nullptr)
        throw std::out_of_range("HashMap: key doesn't exists");

    return tmp->value;
}

//
// Inserts key-value pair into hashmap.
//
template <class K, class V, class F>
void HashMap<K, V, F>::insert(K key, V value)
{
    unsigned i = hash(key);
    // Lock access to table elements at i.
    std::lock_guard<std::mutex> lock(*_mutexes[i]);

    if (_table[i] == nullptr)
    {
        _table[i] = new Element(key, value);
    }
    else
    {
        Element *tmp = _table[i];

        // Traverse the list and check if a key already exists.

        while (tmp->next != nullptr && tmp->key != key)
            tmp = tmp->next;

        // If key exists, change the value, otherwise add new element to end of
        // list.

        if (tmp->key == key)
            tmp->value = value;
        else
            tmp->next = new Element(key, value);
    }
}

//
// Removes key and corresponding value from hashmap. If key doesn't exists
// it throws "out of range" exception.
//
template <class K, class V, class F>
void HashMap<K, V, F>::remove(K key)
{
    unsigned i = hash(key);
    // Lock access to table elements at i.
    std::lock_guard<std::mutex> lock(*_mutexes[i]);
    Element *tmp = _table[i];
    Element *prev = nullptr;

    while (tmp != nullptr && tmp->key != key)
    {
        prev = tmp;
        tmp = tmp->next;
    }

    if (tmp == nullptr)
        throw std::out_of_range("HashMap: key doesn't exists");

    if (prev == nullptr)
        _table[i] = tmp->next;
    else
        prev->next = tmp->next;

    delete tmp;
}

template <class K, class V, class F>
void HashMap<K, V, F>::resize(size_t newSize)
{
    Element **newTable = new Element *[newSize];

    // Populate the new table.
    for (unsigned i = 0; i < _size; ++i)
    {
        // Lock access to table elements at i.
        std::lock_guard<std::mutex> lock(*_mutexes[i]);

        if (_table[i] == nullptr)
            continue;

        Element *tmp = _table[i];

        while (tmp != nullptr)
        {
            Element *old = tmp;

            unsigned newIdx = hashFunctor(tmp->key) % newSize;
            if (newTable[newIdx] == nullptr)
            {
                newTable[newIdx] = new Element(tmp->key, tmp->value);
            }
            else
            {
                Element *p = newTable[newIdx];

                // Traverse the list and check if a key already exists.

                while (p->next != nullptr && p->key != tmp->key)
                    p = p->next;

                // If key exists, change the value, otherwise add new element to
                // end of list.

                if (p->key == tmp->key)
                    p->value = tmp->value;
                else
                    p->next = new Element(tmp->key, tmp->value);
            }

            tmp = tmp->next;
            delete old;
        }

        delete _mutexes[i];
    }

    // Finally deallocate old table and mutexes.
    delete [] _table;
    delete [] _mutexes;

    _size = newSize;
    _table = newTable;
    //mutexes = (std::mutex **) malloc(newSize * sizeof(std::mutex *));
    _mutexes = new std::mutex *[newSize];
}

//
// Prints out hashmap.
//
template <class K, class V, class F>
void HashMap<K, V, F>::print()
{
    for (unsigned i = 0; i < _size; ++i)
    {
        // Lock access to table elements at i.
        std::lock_guard<std::mutex> lock(*_mutexes[i]);

        if (_table[i] == nullptr)
            continue;

        Element *tmp = _table[i];

        std::cout << "[" << i << "] -> ";
        while (tmp != nullptr)
        {
            std::cout << "(" << tmp->key << ", "
                      << tmp->value << "), ";
            tmp = tmp->next;
        }
        std::cout << "" << std::endl;
    }
}

#endif
