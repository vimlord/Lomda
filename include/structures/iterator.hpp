#ifndef _STRUCTURES_ITERATOR_HPP
#define _STRUCTURES_ITERATOR_HPP

template<typename V> class Iterator;

template<typename V>
class Iterable {
    public:
        virtual Iterator<V>* iterator() = 0;
};

// Most data structures are expected to have some form of iterator
template<typename V>
class Iterator {
    public:
        // Gets the current key
        virtual bool hasNext() = 0;
        virtual V next() = 0;

};

#endif
