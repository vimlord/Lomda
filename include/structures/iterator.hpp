#ifndef _STRUCTURES_ITERATOR_HPP
#define _STRUCTURES_ITERATOR_HPP

template<class K, class V> class Iterator;

template<class K, class V>
class Iterable {
    public:
        virtual Iterator<K,V>* iterator() = 0;
};

// Most data structures are expected to have some form of iterator
template<class K, class V>
class Iterator {
    public:
        // Gets the current key
        virtual bool hasNext() = 0;
        virtual V next() = 0;

};

#endif
