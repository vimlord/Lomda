#ifndef _BASELANG_TYPES_HPP_
#define _BASELANG_TYPES_HPP_

#include "stringable.hpp"
#include "reffable.hpp"

#include <unordered_map>

class TypeEnv;
typedef TypeEnv* Tenv;

class Type : public Stringable {
    public:
        virtual ~Type() {}

        virtual Type* clone() = 0;

        /**
         * Performs unification between two supposedly equal
         * types under a typing context. Returns NULL on failure
         * else a copy of the type.
         */
        virtual Type* unify(Type*, Tenv) = 0;

        /**
         * Performs simplification on a type, creating a new Type
         * hierarchy in the process.
         */
        virtual Type* simplify(Tenv) { return clone(); }
        
        // Whether or not some part of the type is still undefined.
        virtual bool isConstant(Tenv) { return true; }

        virtual bool equals(Type*, Tenv) { return false; }
        virtual bool depends_on_tvar(std::string, Tenv) { return false; }
};

class TypeEnv : public Stringable {
    private:
        // The types of all known variables
        std::unordered_map<std::string, Type*> types;
        
        // The most general unification result
        std::unordered_map<std::string, Type*> mgu;

        // The next type variable name to assign
        static std::string next_id;

    public:
        TypeEnv() {}
        ~TypeEnv();

        Type* apply(std::string x);
        bool hasVar(std::string x);
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
        void set_tvar(std::string v, Type *t);
        void rem_tvar(std::string v);
        Type* make_tvar();

        TypeEnv* unify(TypeEnv* other, TypeEnv* scope);

        TypeEnv* clone();
        std::string toString();

        static void reset_tvars() { next_id = "a"; }
        
};
typedef TypeEnv* Tenv;

#endif
