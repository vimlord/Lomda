#ifndef _STRUCTURES_LIST_HPP_
#define _STRUCTURES_LIST_HPP_

#include "structures/map.hpp"
#include "structures/iterator.hpp"

#include <iostream>

template<typename T>
class List : public Map<int, T>, public Iterable<T> {
    public:
        virtual ~List() {}

        virtual bool isEmpty() = 0;

        bool hasKey(int i) { return i >= 0 && i < this->size(); }
};

template<typename T>
class LLnode;

template<typename T>
class LinkedList : public List<T> {
    private:
        LLnode<T> *head;
        class LLiterator : public Iterator<T> {
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

        Iterator<T>* iterator() { return new LLiterator(this); }
};

template<typename T>
class ArrayList : public List<T> {
    private:
        T *arr;
        int N;
        int alen;

        class ALiterator : public Iterator<T> {
            private:
                int idx;
                ArrayList *list;
            public:
                ALiterator(ArrayList *lst) : list(lst) { idx = 0; }
                ALiterator() { idx = 0; }

                bool hasNext() { return idx < list->N; }
                T next() { return list->arr[idx++]; }
        };
    public:

        /**
         * Creates the default, empty arraylist.
         */
        ArrayList() {
            // The array is initalized as empty.
            arr = new T[1];

            // The array is empty.
            N = 0;
            
            // There is one slot allocated at the start.
            alen = 1;
        }
        
        /**
         * Allocates the arraylist with a given data.
         * @param array The data block to store.
         * @param n The length of the block (preferably > 0, lest fate be tempted)
         */
        ArrayList(T *array, int n) {
            // Use the new array
            arr = array;

            // The array is considered to be full.
            N = alen = n;
        }

        ~ArrayList() {
            delete[] arr;
        }

        void add(int, T);
        T get(int i);
        T remove(int);
        void set(int, T);

        int size() { return N; }

        bool isEmpty() { return !N; }

        Iterator<T>* iterator() { return new ALiterator(this); }
};


// C++ is jank AF
#include <cstddef>
#include <stdexcept>
#include <string>

template<typename T>
struct LLnode {
    public:
        LLnode(T v, LLnode* n = NULL) {
            val = v;
            next = n;
        }

        T val;
        struct LLnode *next;
};

template<typename T>
LinkedList<T>::LinkedList() {
    head = NULL;
}

template<typename T>
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

template<typename T>
LinkedList<T>::~LinkedList() {
    while (head) remove(0);
}

template<typename T>
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

template<typename T>
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

template<typename T>
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

template<typename T>
int LinkedList<T>::size() {
    LLnode<T> *node = head;
    int len;

    for (len = 0; node; len++)
        node = node->next;

    return len;
}

template<typename T>
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

template<typename T>
bool LinkedList<T>::isEmpty() { return head == NULL; }

template<typename T>
void ArrayList<T>::add(int idx, T t) {
    if (idx < 0 || idx > N)
        throw std::out_of_range(std::to_string(idx));
    else if (!arr) {
        arr = new T[1];
        alen = 1;
        arr[0] = t;
    } else {
        if (N == alen) {
            // We must extend the memory.
            T *dta;
            if (alen) {
                // We must extend the memory.
                dta = new T[(alen *= 2)];
            } else {
                // Initialize a slot.
                dta = new T[(alen = 1)];
            }
            
            for (int i = 0; i < N; i++)
                dta[i] = arr[i];

            delete[] arr;
            arr = dta;

        }

        // Now, we shift the values over
        for (int i = N; i > N; i--)
            arr[i] = arr[i-1];

        // Finally, add the item and increment.
        arr[N++] = t;
    }
}


template<typename T>
T ArrayList<T>::get(int idx) {
    if (idx < 0 || idx >= N)
        throw std::out_of_range(std::to_string(idx));
    else
        return arr[idx];
}

template<typename T>
T ArrayList<T>::remove(int idx) {
    if (idx < 0 || idx >= N)
        throw std::out_of_range(std::to_string(idx));
    else if (4*N == alen) {
        // In this case, we will shift the values over in a new memory block
        T *blk = new T[2*N];
        for (int i = 0; i < idx; i++)
            blk[i] = arr[i];

        T res = arr[idx];
        N--;

        for (int i = idx; i < N; i++)
            blk[i] = arr[i+1];

        delete[] arr;
        arr = blk;
        
        return res;
    } else {
        T res = arr[idx];
        N--;

        for (int i = idx; i < N; i++)
            arr[i] = arr[i+1];

        return res;
    }
}

template<typename T>
void ArrayList<T>::set(int idx, T t) {
    if (idx < 0 || idx >= N)
        throw std::out_of_range(std::to_string(idx));
    else
        arr[idx] = t;
}

#endif
