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

// Composite types
class LambdaType : public Type {
    private:
        Type *left;
        Type *right;
    public:
        LambdaType(Type *a, Type *b) : left(a), right(b) {}
        ~LambdaType() { delete left; delete right; }
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
class TupleType : public Type {
    private:
        Type *left;
        Type *right;
    public:
        TupleType(Type *a, Type *b) : left(a), right(b) {}
        ~TupleType() { delete left; delete right; }
        Type* clone() { return new TupleType(left->clone(), right->clone()); }

        std::string toString() { return "(" + left->toString() + " * " + right->toString() + ")"; }
};

class BoolType : public Type {
    public:
        BoolType() {}
        Type* clone() { return new BoolType; }
        std::string toString() { return "B"; }
};
class RealType : public Type {
    public:
        RealType() {}
        virtual Type* clone() { return new RealType; }
        virtual std::string toString() { return "R"; }
};
class IntType : public RealType {
    public:
        IntType() {}
        Type* clone() { return new IntType; }
        std::string toString() { return "Z"; }
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
    public:
        VarType(std::string v, Type *t = NULL) : name(v), type(t) {}
        Type* clone() { return new VarType(name, type); }
        std::string toString() { return type ? type->toString() : name; }
};

#endif
