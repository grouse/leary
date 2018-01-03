/**
 * file:    hash_table.h
 * created: 2018-01-03
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

#ifndef LEARY_HASHTABLE_H
#define LEARY_HASHTABLE_H

#define TABLE_SIZE 128

template <typename K, typename V>
struct Pair {
    K key;
    V value;
};

template <typename K, typename V>
struct HashTable {

    // TODO(jesper): replace this with custom array implementation, I don't want
    // to use the generalised array implementation here.
    Array<Pair<K, V>> table[TABLE_SIZE];
};

#define RH_INITIAL_SIZE (128)
#define RH_LOAD_FACTOR  (70)

template<typename K, typename V>
struct RHHashMap {
    struct Entry {
        K   key      = {};
        V   value    = {};
        i32 distance = -1;
    };

    Allocator *allocator = nullptr;
    Entry     *entries   = nullptr;

    i32 capacity         = 0;
    i32 count            = 0;
    i32 resize_threshold = 0;
    u32 mask             = 0;
};

#endif // LEARY_HASHTABLE_H

