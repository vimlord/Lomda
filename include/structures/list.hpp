#ifndef _STRUCTURES_LIST_HPP_
#define _STRUCTURES_LIST_HPP_

#include "structures/map.hpp"
#include "structures/iterator.hpp"

template<class T>
class List : public Map<int, T>, public Iterable<int, T> {
    public:
        virtual bool isEmpty() = 0;

        bool hasKey(int i) { return i >= 0 && i < this->size(); }
};

template<class T>
class LLnode;

template<class T>
class LinkedList : public List<T> {
    private:
        struct LLnode<T> *head;
        class LLiterator : public Iterator<int, T> {
            private:
                LLnode<T> *node;
            public:
                LLiterator(LinkedList *list) : node(list->head) {}
                ~LLiterator() {}

                bool hasNext() { return node != (void*) 0; }
                T next() {
                    T v = node->val;
                    node = node->next;
                    return v;
                }

        };
    public:
        LinkedList();
        LinkedList(T[], int);
        ~LinkedList();

        void add(int, T);
        T get(int);
        T remove(int);
        void set(int, T);

        int size();

        bool isEmpty();

        Iterator<int,T>* iterator() { return new LLiterator(this); }
};


// C++ is jank AF
#include <cstddef>
#include <stdexcept>
#include <string>

template<class T>
struct LLnode {
    public:
        LLnode(T v, LLnode* n = NULL) {
            val = v;
            next = n;
        }

        T val;
        struct LLnode *next;
};

template<class T>
LinkedList<T>::LinkedList() {
    head = NULL;
}

template<class T>
LinkedList<T>::LinkedList(T vs[], int n) {
    head = NULL;

    // Add all n elements
    LLnode<T> node = head;
    for (int i = 0; i < n; i++)
        if (!head)
            node = head = new LLnode<T>(vs[i]);
        else
            node = node->next = new LLnode<T>(vs[i]);
}

template<class T>
LinkedList<T>::~LinkedList() {
    while (head) remove(0);
}

template<class T>
void LinkedList<T>::add(int idx, T val) {
    
    if (head == NULL) {
        // List is empty
        head = new LLnode<T>(val);
    } else if (idx < 0) {
        // Do not allow negative indices
        throw std::out_of_range(std::to_string(idx));
    } else if (idx == 0) {
        // Front of the list
        head = new LLnode<T>(val, head);
    } else {
        // Get the node to attach to
        LLnode<T> *node = head;
        for (; node && --idx;)
            node = node->next;
        
        // Check bounds
        if (!node)
            throw std::out_of_range(std::to_string(idx));
        
        // Then, link up
        node->next = new LLnode<T>(val, node->next);
    }
}

template<class T>
T LinkedList<T>::get(int idx) {
    if (!head)
        throw std::out_of_range(std::to_string(idx));
    else {
        LLnode<T> *node = head;
        while (idx-- && node)
            node = node->next;

        if (!node)
            throw std::out_of_range(std::to_string(idx));
        else
            return node->val;
    }
}

template<class T>
void LinkedList<T>::set(int idx, T val) {
    if (!head)
        throw std::out_of_range(std::to_string(idx));
    else {
        LLnode<T> *node = head;
        while (idx-- && node)
            node = node->next;

        if (!node)
            throw std::out_of_range(std::to_string(idx));
        else
            node->val = val;
    }
}

template<class T>
int LinkedList<T>::size() {
    LLnode<T> *node = head;
    int len;

    for (len = 0; node; len++)
        node = node->next;

    return len;
}

template<class T>
T LinkedList<T>::remove(int idx) {
    if (!head || idx < 0)
        throw std::out_of_range(std::to_string(idx));
    else if (idx == 0) {
        T val = head->val;
        LLnode<T> *node = head;
        head = head->next;

        delete node;

        return val;

    } else {
        // Get the node before the sought index
        LLnode<T> *node = head;
        while (--idx && node)
            node = node->next;
        
        // Check bounds
        if (!node)
            throw std::out_of_range(std::to_string(idx));
        
        // Get the next node, and verify its existence
        LLnode<T> *n = node->next;
        if (!n)
            throw std::out_of_range(std::to_string(idx));
        
        // Get values and relink
        T val = n->val;
        node->next = n->next;

        delete n;

        return val;

    }
}

template<class T>
bool LinkedList<T>::isEmpty() { return head == NULL; }



#endif
