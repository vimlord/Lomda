#ifndef _BASELANG_TYPES_HPP_
#define _BASELANG_TYPES_HPP_

#include "stringable.hpp"
#include "reffable.hpp"

#include <unordered_map>

class TypeEnv;
typedef TypeEnv* Tenv;

class Type : public Stringable {
    public:
        virtual Type* clone() = 0;
        virtual Type* unify(Type*, Tenv) = 0;
};

class TypeEnv : public Stringable {
    private:
        // The types of all known variables
        std::unordered_map<std::string, Type*> types;
        
        // The most general unification result
        std::unordered_map<std::string, Type*> mgu;
    public:
        TypeEnv() {}
        ~TypeEnv();

        Type* apply(std::string x);
        int set(std::string x, Type* v);

        int remove(std::string x) {
            if (types.find(x) != types.end()) {
                delete types[x];
                types.erase(x);
                return 0;
            } else return 1;
        }

        void register_tvar(Type *v) { mgu[v->toString()] = v; }
        Type* get_tvar(std::string v);
        void set_tvar(std::string v, Type *t) { rem_tvar(v); mgu[v] = t; }
        void rem_tvar(std::string v);

        TypeEnv* clone();
        std::string toString();
        
};
typedef TypeEnv* Tenv;

#endif
