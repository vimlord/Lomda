#ifndef _SVRUCVURES_HASHMAP_HPP_
#define _SVRUCVURES_HASHMAP_HPP_

#include <functional>

#include "structures/map.hpp"
#include "structures/list.hpp"

template<typename K, typename V>
class HashMap : public Map<K, V> {
    private:
        // Define a secret datatype for representing the contained array.
        struct slot {
            bool filled;
            K key;
            V val;
        };

        slot *arr;
        
        // Size of the array.
        int arrlen;

        // Number of elements in the map.
        int N = 0;
        
        class Miterator : public Iterator<std::string> {
            private:
                int arrlen;
                int idx = 0;
                slot *arr;
            public:
                Miterator(HashMap<K,V> *map) : arrlen(map->arrlen), arr(map->arr) {
                    // Progress the pointer to the first element.
                    while (idx < arrlen && !arr[idx].filled) idx++;
                }

                std::string next() {
                    // Grab the value.
                    K key = arr[idx].key;
                    
                    // Progress the pointer to the next element.
                    while (++idx < arrlen && !arr[idx].filled);
                    
                    // Yield a result.
                    return key;
                }

                bool hasNext() {
                    // The cursor needs to be in bounds.
                    return idx < arrlen;
                }
        };

    public:
        /**
         * Creates an empty hashmap with a given size.
         */
        HashMap(int = 16);
        ~HashMap();
        
        V get(K);
        V remove(K);

        void add(K, V);
        void set(K, V);

        bool hasKey(K);

        int size() { return N; }

        Iterator<std::string> *iterator() { return new Miterator(this); }
};



template<typename K, typename V>
HashMap<K, V>::HashMap(int n) {
    // Allocate slots.
    arr = new slot[(arrlen = n)];

    // Zero the slots
    for (int i = 0; i < arrlen; i++)
        arr[i].filled = false;
}

template<typename K, typename V>
HashMap<K, V>::~HashMap() {
    delete[] arr;
}

template<typename K, typename V>
inline V HashMap<K,V>::get(K key) {
    // Hash the element.
    int idx = std::hash<K>{}(key) % arrlen;

    // Iterate through the hashmap.
    for (int i = 0; i < arrlen; i++, idx++) {
        // Wrap if need be.
        if (idx >= arrlen) idx = 0;
        
        // If we found the slot, return.
        if (arr[idx].filled && arr[idx].key == key)
            return arr[idx].val;
    }
    
    // The element does not exist.
    throw std::out_of_range(std::to_string(idx));
}

template<typename K, typename V>
inline V HashMap<K,V>::remove(K key) {
    // Hash the element.
    int idx = std::hash<K>{}(key) % arrlen;

    // Iterate through the hashmap.
    for (int i = 0; i < arrlen; i++, idx++) {
        // Wrap if need be.
        if (idx >= arrlen) idx = 0;
        
        // If we found the slot, return.
        if (arr[idx].filled && arr[idx].key == key) {
            // Remove the item.
            arr[idx].filled = false;
            N--;
            return arr[idx].val;
        }
    }
    
    // The element does not exist.
    throw std::out_of_range(std::to_string(idx));
}

template<typename K, typename V>
inline void HashMap<K,V>::add(K key, V val) {
    if (N == arrlen) {
        // The table must be expanded. We expand by the Golden Ratio
        // because this will allow the freed block to be reused should
        // another realloc attempt to use it.
        slot *newarr = new slot[(arrlen *= 2)];
        for (int i = 0; i < arrlen; i++) newarr[i].filled = false;
        
        // Put the new array in, keeping the old array.
        slot *tmp = arr;
        arr = newarr;
        
        // Move all of the blocks over
        for (int i = 0; i < N; i++) {
            add(tmp[i].key, tmp[i].val);
        }
        
        // Use the new array.
        delete[] tmp;
    }

    // Hash the element.
    int idx = std::hash<K>{}(key) % arrlen;
    
    // Iterate through the hashmap to find an empty slot. One must exist.
    while (arr[idx].filled) {
        // Wrap if need be.
        if (++idx >= arrlen) idx = 0;
    }

    // Insert at the index
    arr[idx].key = key;
    arr[idx].val = val;
    arr[idx].filled = true;

    N++;

}

template<typename K, typename V>
inline void HashMap<K,V>::set(K key, V val) {
    // Hash the element.
    int idx = std::hash<K>{}(key) % arrlen;

    // Iterate through the hashmap.
    for (int i = 0; i < arrlen; i++, idx++) {
        // Wrap if need be.
        if (idx >= arrlen) idx = 0;
        
        // If we found the slot, return.
        if (arr[idx].filled && arr[idx].key == key)
            arr[idx].val = val;
    }
    
    // The element does not exist.
    throw std::out_of_range(std::to_string(idx));
}

template<typename K, typename V>
inline bool HashMap<K,V>::hasKey(K key) {
    // Hash the element.
    int idx = std::hash<K>{}(key) % arrlen;

    // Iterate through the hashmap.
    for (int i = 0; i < arrlen; i++, idx++) {
        // Wrap if need be.
        if (idx >= arrlen) idx = 0;
        
        // If we found the slot, return.
        if (arr[idx].filled && arr[idx].key == key)
            return true;
    }

    return false;
}

#endif
