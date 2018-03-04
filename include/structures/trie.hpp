#ifndef _STRUCTURES_TRIE_HPP_
#define _STRUCTURES_TRIE_HPP_

#include "structures/list.hpp"

#include <cstddef>
#include <stdexcept>
#include <iostream>

template<typename V>
class TrieNode {
    public:
        ArrayList<TrieNode<V>*> *paths = NULL;
        ArrayList<char> *labels = NULL;
        TrieNode<V> *root = NULL;

        V val;
        bool hasVal = false;
        
        TrieNode(V v) {
            paths = new ArrayList<TrieNode<V>*>;
            labels = new ArrayList<char>;
            val = v;
            hasVal = true;
        }
        TrieNode() {
            paths = new ArrayList<TrieNode<V>*>;
            labels = new ArrayList<char>;
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

template<typename V>
class Trie : public Map<std::string, V> {
    private:
        struct TrieNode<V> *head;

        class Titerator : public Iterator<std::string> {
            private:
                TrieNode<V> *node;

                Iterator<char> *kit = NULL;
                Iterator<TrieNode<V>*> *pit = NULL;

                Iterator<std::string> *subit = NULL;
                
                std::string c;

            public:
                Titerator(Trie<V> *t) {
                    node = t->head;
                }
                Titerator(TrieNode<V> *n) : node(n) {}
                ~Titerator() {
                    delete kit;
                    delete pit;
                    delete subit;
                }

                std::string next() {

                    std::string res;

                    if (!kit) {
                        bool found = false;

                        kit = node->labels->iterator();
                        pit = node->paths->iterator();
                        
                        // If the node has a value, then return it
                        if (node->hasVal) {
                            res = "";
                            found = true;
                        }
                        
                        if (pit->hasNext()) {
                            subit = new Titerator(pit->next());
                            char ch = kit->next();

                            c = "";
                            c.append(1, ch);

                            if (!found) {
                                auto sub = subit->next();
                                res = c + sub;
                            }
                        }
                    } else {
                        if (!subit->hasNext()) {
                            delete subit;
                            subit = new Titerator(pit->next());
                            c = kit->next();
                        }

                        
                        res = c + subit->next();
                    }

                    return res;
                }
                bool hasNext() {
                    return node && (!kit || kit->hasNext() || (subit && subit->hasNext()));
                }
        };

    public:
        Trie() : head(NULL) {}
        ~Trie() { delete head; }

        V get(std::string);
        V remove(std::string);

        void add(std::string, V);
        void set(std::string, V);

        bool hasKey(std::string);
        
        int size();

        Iterator<std::string> *iterator() { return new Titerator(this); }
};

template<typename V>
V Trie<V>::get(std::string key) {
    auto node = head;

    for (int i = 0; key[i] && node; i++) {
        auto it = node->labels->iterator();
        
        int j;
        for (j = 0; it->hasNext(); j++)
            if (it->next() == key[i]) {
                break;
            }
        
        // Iterative garbage collection
        delete it;

        if (j < node->labels->size()) {
            // Get the correct child
            node = node->paths->get(j);
        } else
            throw std::out_of_range(key);
    }

    if (node && node->hasVal)
        return node->val;
    else
        throw std::out_of_range(key);
}

template<typename V>
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
            node = node->paths->get(j);
        else
            throw std::out_of_range(key);
    }

    if (node && node->hasVal)
        node->val = val;
    else
        throw std::out_of_range(key);
}
template<typename V>
void Trie<V>::add(std::string key, V val) {
    auto node = head;
    
    int i;
    for (i = 0; i < key.length() && node; i++) {
        auto it = node->labels->iterator();
        
        int j;
        for (j = 0; it->hasNext(); j++)
            if (it->next() == key[i]) {
                break;
            }
        
        // Iterative garbage collection
        delete it;

        if (j < node->labels->size()) {
            // Get the correct child
            node = node->paths->get(j);
        } else
            // We can add a branch
            break;
    }

    
    if (key[i] == '\0') {
        // The key already exists.
        node->val = val;
        node->hasVal = true;
    } else {
        // We will build a node chain. Set a new head if need be
        TrieNode<V> *tail = NULL;
        if (head)
            tail = node;
        else
            tail = head = new TrieNode<V>;

        for (int j = i; j < key.length(); j++) {
            auto tmp = new TrieNode<V>;

            tail->labels->add(0, key[j]);
            tail->paths->add(0, tmp);
            
            tail = tmp;
        }
        
        tail->hasVal = true;
        tail->val = val;
    }
}

template<typename V>
V Trie<V>::remove(std::string key) {
    auto node = head;

    for (int i = 0; key[i] && node; i++) {
        auto it = node->labels->iterator();
        
        int j;
        for (j = 0; it->hasNext(); j++) {
            if (it->next() == key[i]) {
                break;
            }
        }
        
        // Iterative garbage collection
        delete it;

        if (j < node->labels->size()) {
            // Get the correct child
            node = node->paths->get(j);
        } else {
            throw std::out_of_range(key);
        }
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
                head = NULL;
            } else for (int j = 0; j < node->labels->size(); j++) {
                if (node->labels->get(j) == key[i]) {
                    delete node->paths->remove(j);
                    node->labels->remove(j);
                    break;
                }
            }

            return res;
        }
    } else
        throw std::out_of_range(key);
}

template<typename V>
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
            node = node->paths->get(j);
        else
            return false;
    }

    return node && node->hasVal;
}

template<typename V>
int Trie<V>::size() {
    if (head) return head->size();
    else return 0;
}


#endif
