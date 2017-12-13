#ifndef _STRUCTURES_MAP_HPP_
#define _STRUCTURES_MAP_HPP_

template<class K, class V>
class Map {
    public:
        virtual V get(K) = 0;
        virtual V remove(K) = 0;

        virtual void add(K, V) = 0;
        virtual void set(K, V) = 0;

        virtual bool hasKey(K) = 0;

        virtual int size() = 0;
};

template<class K, class V>
class Tnode;

#endif
