#ifndef _STRUCTURES_TRIE_HPP_
#define _STRUCTURES_TRIE_HPP_

#include "structures/list.hpp"

template<class V>
struct TrieNode;

template<class V>
class Trie : public Map<std::string, V> {
    private:
        struct TrieNode<V> *head = NULL;
    
    public:
        Trie() {}
        ~Trie() { delete head; }

        V get(std::string);
        V remove(std::string);

        void add(std::string, V);
        void set(std::string, V);

        bool hasKey(std::string);
        
        int size();
};


#include <cstddef>
#include <stdexcept>

template<class V>
class TrieNode {
    public:
        TrieNode<V> *root = NULL;
        ArrayList<char> *labels;
        ArrayList<TrieNode<V>*> *paths;

        V val;
        bool hasVal;
        
        TrieNode(V v) {
            val = v;
            paths = new ArrayList<TrieNode<V>>;
            labels = new ArrayList<char>;
            hasVal = true;
        }
        TrieNode() {
            paths = new ArrayList<TrieNode<V>>;
            labels = new ArrayList<char>;
            hasVal = false;
        }
        ~TrieNode() {
            delete labels;

            auto it = paths->iterator();
            while (it->hasNext()) delete it->next();
            delete it;

            delete paths;
        }

        int size() {
            int n = hasVal ? 1 : 0;
            
            auto it = paths->iterator();
            while (it->hasNext())
                n += it->next()->size();

            delete it;

            return n;
        }
};

template<class V>
V Trie<V>::get(std::string key) {
    auto node = head;

    for (int i = 0; key[i] && node; i++) {
        auto it = node->labels->iterator();
        
        int j;
        for (j = 0; it->hasNext(); j++)
            if (it->next() == key[i])
                break;
        
        // Iterative garbage collection
        delete it;

        if (j < node->labels->size())
            // Get the correct child
            node = node->get(j);
        else
            throw std::out_of_range(key);
    }

    if (node && node->hasVal)
        return node->val;
    else
        throw std::out_of_range(key);
}

template<class V>
void Trie<V>::set(std::string key, V val) {
    auto node = head;

    for (int i = 0; key[i] && node; i++) {
        auto it = node->labels->iterator();
        
        int j;
        for (j = 0; it->hasNext(); j++)
            if (it->next() == key[i])
                break;
        
        // Iterative garbage collection
        delete it;

        if (j < node->labels->size())
            // Get the correct child
            node = node->get(j);
        else
            throw std::out_of_range(key);
    }

    if (node && node->hasVal)
        node->val = val;
    else
        throw std::out_of_range(key);
}

template<class V>
void Trie<V>::add(std::string key, V val) {
    auto node = head;
    
    int i;
    for (int i = 0; key[i] && node; i++) {
        auto it = node->labels->iterator();
        
        int j;
        for (j = 0; it->hasNext(); j++)
            if (it->next() == key[i])
                break;
        
        // Iterative garbage collection
        delete it;

        if (j < node->labels->size())
            // Get the correct child
            node = node->get(j);
        else
            // We can add a branch
            break;
    }
    
    if (key[i]) {
        // The key already exists.
        node->val = val;
        node->hasVal = true;
    } else {
        // We will build a node chain.
        auto tail = new TrieNode<V>(val);

        for (int j = key.length()-1; j >= i; j--) {
            auto tmp = new Trie<V>;

            tmp->labels->add(0, key[j]);
            tmp->paths->add(0, tail);

            tail = tail->root = tmp;
        }
    }
}

template<class V>
V Trie<V>::remove(std::string key) {
    auto node = head;

    for (int i = 0; key[i] && node; i++) {
        auto it = node->labels->iterator();
        
        int j;
        for (j = 0; it->hasNext(); j++)
            if (it->next() == key[i])
                break;
        
        // Iterative garbage collection
        delete it;

        if (j < node->labels->size())
            // Get the correct child
            node = node->get(j);
        else
            throw std::out_of_range(key);
    }

    if (node && node->hasVal) {
        auto res = node->val;
        
        // Perform removal
        node->hasVal = false;
        
        if (node->labels->isEmpty()) {
            // We can remove a node
            int i = key.length();
            while (node && !node->hasVal && node->labels->size() == 1) {
                node = node->root;
                i--;
            }

            // Now, the node is either null (empty list), has a value, or branches
            if (!node) {
                delete head;
                return res;
            }
            
            auto it = node->labels->iterator();
            for (int j = 0; it->hasNext(); j++)
                if (it->next() == key[i]) {
                    delete it;
                    
                    // Kill the subpath
                    delete node->paths->remove(j);
                    node->labels->remove(j);
                    
                    return res;
                }

            throw std::out_of_range(key);
        }
    } else
        throw std::out_of_range(key);
}

template<class V>
bool Trie<V>::hasKey(std::string key) {
    auto node = head;

    for (int i = 0; key[i] && node; i++) {
        auto it = node->labels->iterator();
        
        int j;
        for (j = 0; it->hasNext(); j++)
            if (it->next() == key[i])
                break;
        
        // Iterative garbage collection
        delete it;

        if (j < node->labels->size())
            // Get the correct child
            node = node->get(j);
        else
            throw std::out_of_range(key);
    }

    return node && node->hasVal;
}

template<class V>
int Trie<V>::size() {
    if (head) return head->size();
    else return 0;
}


#endif
