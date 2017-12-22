// The MIT License (MIT)
//
// Thread-safe generic hashmap
// Copyright (c) 2016-2017 Jozef Kolek <jkolek@gmail.com>
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
class HashMapIter;

template <class K, class V>
class HashMapElement
{
    K _key;
    V _value;
public:
    HashMapElement(K key, V value) : _key(key), _value(value) {}

    K getKey() { return _key; }
    V getValue() { return _value; }
    void setValue(V value) { _value = value; }

    HashMapElement<K, V> *next = nullptr;
};

template <class K, class V, class F>
class HashMap
{
    // Table size
    size_t _size;

    //
    // The table elements are pointers to HashMapElement. Table contains linked
    // lists where elements of the list represents entries with key-value pairs.
    //
    HashMapElement<K, V> **_table;

    //
    // We have one mutex for each index of table, so multiple indexes can be
    // accessed at the same time.
    //
    std::mutex **_mutexes;

    //
    // Hash function is actually a class used as functor. This function
    // calculates an index where an element needs to be stored. The calculated
    // index is modulo of MAX_TABLE_SIZE.
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
    HashMapElement<K, V> **getTable() { return _table; }

    HashMap() : _size(0), _table(nullptr), _mutexes(nullptr) {}
    HashMap(size_t Size);
    HashMap(HashMap &other);             // Copy constructor
    HashMap(HashMap &&other);            // Move constructor

    HashMap& operator=(HashMap &other);  // Copy assignment operator
    HashMap& operator=(HashMap &&other); // Move assignment operator

    HashMapIter<K, V, F> *createIterator()
    {
        return new HashMapIter<K, V, F>(this);
    }
};

//
// HashMap Iterator class
//
template <class K, class V, class F>
class HashMapIter
{
    HashMap<K, V, F> *_map;
    HashMapElement<K, V> *_curr = nullptr;
    unsigned _idx = 0;
public:
    HashMapIter(HashMap<K, V, F> *map) : _map(map) {}

    void first()
    {
        size_t size = _map->getSize();
        _idx = 0;
        while (_idx < size && _map->getTable()[_idx] == nullptr)
            _idx++;
        if (_idx < size)
            _curr = _map->getTable()[_idx];
        else
            _curr = nullptr;
    }

    void next()
    {
        if (_curr == nullptr)
            return;

        if (_curr->next == nullptr)
        {
            size_t size = _map->getSize();

            do
            {
                _idx++;
            }
            while (_idx < size && _map->getTable()[_idx] == nullptr);

            if (_idx < size)
                _curr = _map->getTable()[_idx];
            else
                _curr = nullptr;
        }
        else
        {
            _curr = _curr->next;
        }
    }

    bool isEnd()
    {
        return _curr == nullptr;
    }

    HashMapElement<K, V> *getCurrent()
    {
        return _curr;
    }
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
    _table = new HashMapElement<K, V> *[_size];
    _mutexes = new std::mutex *[_size];

    for (unsigned i = 0; i < _size; i++)
        _mutexes[i] = new std::mutex;

    for (unsigned i = 0; i < _size; i++)
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

    for (unsigned i = 0; i < _size; i++)
    {
        {
            // Lock access to table elements at i.
            std::lock_guard<std::mutex> lock(*_mutexes[i]);

            if (_table[i] == nullptr)
                continue;

            HashMapElement<K, V> *tmp = _table[i];

            while (tmp != nullptr)
            {
                HashMapElement<K, V> *old = tmp;
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
    for (unsigned i = 0; i < other._size; i++)
    {
        // Lock access to table elements at i.
        std::lock_guard<std::mutex> lock(*other._mutexes[i]);

        if (other._table[i] == nullptr)
            continue;

        HashMapElement<K, V> *tmp = other._table[i];

        while (tmp != nullptr)
        {
            insert(tmp->getKey(), tmp->getValue());
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
        for (unsigned i = 0; i < other._size; i++)
        {
            // Lock access to table elements at i.
            std::lock_guard<std::mutex> lock(*other._mutexes[i]);

            if (other._table[i] == nullptr)
                continue;

            HashMapElement<K, V> *tmp = other._table[i];

            while (tmp != nullptr)
            {
                insert(tmp->getKey(), tmp->getValue());
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
    unsigned idx = hash(key);
    // Lock access to table elements at idx.
    std::lock_guard<std::mutex> lock(*_mutexes[idx]);
    HashMapElement<K, V> *tmp = _table[idx];

    while (tmp != nullptr && tmp->getKey() != key)
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
    unsigned idx = hash(key);
    // Lock access to table elements at idx.
    std::lock_guard<std::mutex> lock(*_mutexes[idx]);
    HashMapElement<K, V> *tmp = _table[idx];

    while (tmp != nullptr && tmp->getKey() != key)
        tmp = tmp->next;

    if (tmp == nullptr)
        throw std::out_of_range("HashMap: key doesn't exists");

    return tmp->getValue();
}

//
// Inserts key-value pair into hashmap.
//
template <class K, class V, class F>
void HashMap<K, V, F>::insert(K key, V value)
{
    unsigned idx = hash(key);
    // Lock access to table elements at idx.
    std::lock_guard<std::mutex> lock(*_mutexes[idx]);

    if (_table[idx] == nullptr)
    {
        _table[idx] = new HashMapElement<K, V>(key, value);
    }
    else
    {
        HashMapElement<K, V> *tmp = _table[idx];

        // Traverse the list and check if a key already exists.

        while (tmp->next != nullptr && tmp->getKey() != key)
            tmp = tmp->next;

        // If key exists, change the value, otherwise add new element to end of
        // list.

        if (tmp->getKey() == key)
            tmp->setValue(value);
        else
            tmp->next = new HashMapElement<K, V>(key, value);
    }
}

//
// Removes key and corresponding value from hashmap. If key doesn't exists
// it throws "out of range" exception.
//
template <class K, class V, class F>
void HashMap<K, V, F>::remove(K key)
{
    unsigned idx = hash(key);
    // Lock access to table elements at idx.
    std::lock_guard<std::mutex> lock(*_mutexes[idx]);
    HashMapElement<K, V> *tmp = _table[idx];
    HashMapElement<K, V> *prev = nullptr;

    while (tmp != nullptr && tmp->getKey() != key)
    {
        prev = tmp;
        tmp = tmp->next;
    }

    if (tmp == nullptr)
        throw std::out_of_range("HashMap: key doesn't exists");

    if (prev == nullptr)
        _table[idx] = tmp->next;
    else
        prev->next = tmp->next;

    delete tmp;
}

template <class K, class V, class F>
void HashMap<K, V, F>::resize(size_t newSize)
{
    HashMapElement<K, V> **newTable = new HashMapElement<K, V> *[newSize];

    // Populate the new table.
    for (unsigned i = 0; i < _size; i++)
    {
        // Lock access to table elements at i.
        std::lock_guard<std::mutex> lock(*_mutexes[i]);

        if (_table[i] == nullptr)
            continue;

        HashMapElement<K, V> *tmp = _table[i];

        while (tmp != nullptr)
        {
            HashMapElement<K, V> *old = tmp;

            unsigned newIdx = hashFunctor(tmp->getKey()) % newSize;
            if (newTable[newIdx] == nullptr)
            {
                newTable[newIdx] = new HashMapElement<K, V>(tmp->getKey(),
                                                            tmp->getValue());
            }
            else
            {
                HashMapElement<K, V> *p = newTable[newIdx];

                // Traverse the list and check if a key already exists.

                while (p->next != nullptr && p->getKey() != tmp->getKey())
                    p = p->next;

                // If key exists, change the value, otherwise add new element to
                // end of list.

                if (p->getKey() == tmp->getKey())
                    p->setValue(tmp->getValue());
                else
                    p->next = new HashMapElement<K, V>(tmp->getKey(),
                                                       tmp->getValue());
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
    for (unsigned i = 0; i < _size; i++)
    {
        // Lock access to table elements at idx.
        std::lock_guard<std::mutex> lock(*_mutexes[i]);

        if (_table[i] == nullptr)
            continue;

        HashMapElement<K, V> *tmp = _table[i];

        std::cout << "[" << i << "] -> ";
        while (tmp != nullptr)
        {
            std::cout << "(" << tmp->getKey() << ", "
                      << tmp->getValue() << "), ";
            tmp = tmp->next;
        }
        std::cout << "" << std::endl;
    }
}

#endif
