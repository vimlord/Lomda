#ifndef _BASELANG_TYPES_HPP_
#define _BASELANG_TYPES_HPP_

#include "stringable.hpp"
#include "reffable.hpp"

#include <unordered_map>

class Type : public Stringable, public Reffable {
    public:
        virtual Type* clone() = 0;
        virtual Type* unify(Type*) = 0;
};

class TypeEnv : public Stringable, public Reffable {
    private:
        std::unordered_map<std::string, Type*> types;
    public:
        TypeEnv() {}
        ~TypeEnv() {
            for (auto it : types)
                it.second->rem_ref();
        }

        Type* apply(std::string x) { return types.find(x) != types.end() ? types[x] : NULL; }
        int set(std::string x, Type* v);

        int remove(std::string x) {
            if (types.find(x) != types.end()) {
                types[x]->rem_ref();
                types.erase(x);
                return 0;
            } else return 1;
        }

        TypeEnv* clone();
        std::string toString();
        
};
typedef TypeEnv* Tenv;

#endif
