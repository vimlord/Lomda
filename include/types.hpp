#ifndef _TYPES_HPP_
#define _TYPES_HPP_

#include "baselang/types.hpp"
#include "value.hpp"

inline bool val_is_integer(Val v) {
    return typeid(*v) == typeid(IntVal);
}
inline bool val_is_real(Val v) {
    return typeid(*v) == typeid(RealVal);
}
inline bool val_is_number(Val v) {
    return val_is_integer(v) || val_is_real(v);
}


inline bool val_is_list(Val v) {
    return typeid(*v) == typeid(ListVal);
}
inline bool val_is_dict(Val v) {
    return typeid(*v) == typeid(DictVal);
}
inline bool val_is_tuple(Val v) {
    return typeid(*v) == typeid(TupleVal);
}
inline bool val_is_data_struct(Val v) {
    return val_is_list(v) || val_is_dict(v) || val_is_tuple(v);
}


inline bool val_is_bool(Val v) {
    return typeid(*v) == typeid(BoolVal);
}
inline bool val_is_lambda(Val v) {
    return typeid(*v) == typeid(LambdaVal);
}
inline bool val_is_string(Val v) {
    return typeid(*v) == typeid(StringVal);
}

class PairType : public Type {
    protected:
        Type *left;
        Type *right;
    public:
        PairType(Type *a, Type *b) : left(a), right(b) {}
        ~PairType() { delete left; delete right; }

        Type* getLeft() { return left; }
        Type* getRight() { return right; }
};

// Composite primitive types
class LambdaType : public Type {
    private:
        Type *left;
        Type *right;
        Tenv env;
    public:
        LambdaType(Type *a, Type *b, Tenv t = NULL)
            : left(a), right(b), env(t) {}
        ~LambdaType() { delete left; delete right; delete env; }

        Type* getLeft() { return left; }
        Type* getRight() { return right; }
        Tenv getEnv() { return env; }

        Type* clone() { return new LambdaType(left->clone(), right->clone()); }
        Type* unify(Type*, Tenv);

        std::string toString();
};
class ListType : public Type {
    private:
        Type *type;
    public:
        ListType(Type *t) : type(t) {}
        ~ListType() { delete type; }
        Type* clone() { return new ListType(type->clone()); }
        Type* unify(Type*, Tenv);

        Type* subtype() { return type; }

        std::string toString();
};
class TupleType : public PairType {
    public:
        using PairType::PairType;

        Type* clone() { return new TupleType(left->clone(), right->clone()); }
        Type* unify(Type*, Tenv);

        std::string toString();
};

/** Operator types
 * Motivation: We will wish to restrict the types of veriables
 */
class SumType : public PairType {
    public:
        using PairType::PairType;
        Type* clone() { return new SumType(left->clone(), right->clone()); }
        Type* unify(Type*, Tenv);

        std::string toString();
};
class MultType : public PairType {
    public:
        using PairType::PairType;
        Type* clone();
        Type* unify(Type*, Tenv);

        std::string toString();
};

// Primitive types
class PrimitiveType : public Type {};
class BoolType : public PrimitiveType {
    public:
        Type* clone() { return new BoolType; }
        Type* unify(Type*, Tenv);
        std::string toString() { return "B"; }
};
class RealType : public PrimitiveType {
    public:
        virtual Type* clone() { return new RealType; }
        virtual Type* unify(Type*, Tenv);
        virtual std::string toString() { return "R"; }
};
class IntType : public RealType {
    public:
        Type* clone() { return new IntType; }
        Type* unify(Type*, Tenv);
        std::string toString() { return "Z"; }
};
class StringType : public PrimitiveType {
    public:
        Type* clone() { return new StringType; }
        Type* unify(Type*, Tenv);
        std::string toString() { return "S"; }
};
class VoidType : public PrimitiveType {
    public:
        VoidType() {}
        Type* clone() { return new VoidType; }
        Type* unify(Type*, Tenv);
        std::string toString() { return "void"; }
};

// Special types for polymorphism
class VarType : public Type {
    private:
        std::string name;
        static std::string NEXT_ID;
    public:
        VarType() {
            // Progress to the next id
            int i;
            for (i = NEXT_ID.length() - 1; i >= 0 && NEXT_ID[i] == 'z'; i--)
                NEXT_ID[i] = 'a';
            if (i == -1)
                NEXT_ID = "a" + NEXT_ID;
        }
        VarType(std::string v) : name(v) {}
        Type* clone() { return new VarType(name); }
        Type* unify(Type*, Tenv);

        std::string toString() { return name; }
        
        // If we need to set NEXT_ID, then we can do so.
        static void setNextId(std::string s) { NEXT_ID = s; }
};

#endif
