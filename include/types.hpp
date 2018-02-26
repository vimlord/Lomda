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
class LambdaType : public PairType {
    public:
        using PairType::PairType;

        Type* clone() { return new LambdaType(left->clone(), right->clone()); }

        std::string toString() { return "(" + left->toString() + " -> " + right->toString() + ")"; }
};
class ListType : public Type {
    private:
        Type *type;
    public:
        ListType(Type *t) : type(t) {}
        ~ListType() { delete type; }
        Type* clone() { return new ListType(type->clone()); }

        std::string toString() { return "[" + type->toString() + "]"; }
};
class TupleType : public PairType {
    public:
        using PairType::PairType;

        Type* clone() { return new TupleType(left->clone(), right->clone()); }

        std::string toString() { return "(" + left->toString() + " * " + right->toString() + ")"; }
};

/** Operator types
 * Motivation: We will wish to restrict the types of veriables
 */
class SumType : public PairType {
    public:
        using PairType::PairType;
        Type* clone() { return new SumType(left->clone(), right->clone()); }

        std::string toString() { return "(" + left->toString() + " + " + right->toString() + ")"; }
};
class MultType : public PairType {
    public:
        using PairType::PairType;
        Type* clone() { return new SumType(left->clone(), right->clone()); }

        std::string toString() { return "(" + left->toString() + " x " + right->toString() + ")"; }
};

// Primitive types
class BoolType : public Type {
    public:
        Type* clone() { return new BoolType; }
        std::string toString() { return "B"; }
};
class RealType : public Type {
    public:
        virtual Type* clone() { return new RealType; }
        virtual std::string toString() { return "R"; }
};
class IntType : public RealType {
    public:
        Type* clone() { return new IntType; }
        std::string toString() { return "Z"; }
};
class StringType : public Type {
    public:
        Type* clone() { return new StringType; }
        std::string toString() { return "S"; }
};
class VoidType : public Type {
    public:
        VoidType() {}
        Type* clone() { return new VoidType; }
        std::string toString() { return "void"; }
};

// Special types for polymorphism
class VarType : public Type {
    private:
        std::string name;
        Type* type;
        static std::string NEXT_ID;
    public:
        VarType(Type *t = NULL) : VarType(NEXT_ID, t) {
            // Progress to the next id
            int i;
            for (i = NEXT_ID.length() - 1; i >= 0 && NEXT_ID[i] == 'z'; i--)
                NEXT_ID[i] = 'a';
            if (i == -1)
                NEXT_ID = "a" + NEXT_ID;
        }
        VarType(std::string v, Type *t = NULL) : name(v), type(t) {}
        Type* clone() { return new VarType(name, type); }
        
        // Gets the type if it has been found, otherwise NULL
        Type* getType() { return type; }

        std::string toString() { return type ? type->toString() : name; }
        
        // If we need to set NEXT_ID, then we can do so.
        static void setNextId(std::string s) { NEXT_ID = s; }
};

#endif
