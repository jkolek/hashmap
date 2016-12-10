// The MIT License (MIT)
//
// Thread-safe generic hashmap - header file
// Copyright (c) 2016 Jozef Kolek <jkolek@gmail.com>
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

#include "hashmap.cpp"

#endif
