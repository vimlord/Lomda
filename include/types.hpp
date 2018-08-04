#ifndef _TYPES_HPP_
#define _TYPES_HPP_

#include "baselang/types.hpp"
#include "value.hpp"

#include <initializer_list>
#include <utility>

template<typename T>
inline bool isType(const Type* t) {
    return t && dynamic_cast<const T*>(t) != nullptr;
}

inline bool val_is_integer(Val v) {
    return isVal<IntVal>(v);
}
inline bool val_is_real(Val v) {
    return isVal<RealVal>(v);
}
inline bool val_is_number(Val v) {
    return val_is_integer(v) || val_is_real(v);
}


inline bool val_is_list(Val v) {
    return isVal<ListVal>(v);
}
inline bool val_is_dict(Val v) {
    return isVal<DictVal>(v);
}
inline bool val_is_tuple(Val v) {
    return isVal<TupleVal>(v);
}
inline bool val_is_data_struct(Val v) {
    return val_is_list(v) || val_is_dict(v) || val_is_tuple(v);
}


inline bool val_is_bool(Val v) {
    return isVal<BoolVal>(v);
}
inline bool val_is_lambda(Val v) {
    return isVal<LambdaVal>(v);
}
inline bool val_is_string(Val v) {
    return isVal<StringVal>(v);
}

/**
 * Defines an Algebraic Datatype.
 */
class AlgebraicDataType : public Type {
    private:
        // The name of the ADT
        std::string name;
        
        // The kinds of this ADT
        std::string *kinds;

        // The composite types of each ADT
        Type* **argss;
    public:
        AlgebraicDataType(std::string nm, std::string *ks, Type ***vs)
        : name(nm), kinds(ks), argss(vs) {}
        AlgebraicDataType(std::string nm)
        : name(nm), kinds(NULL), argss(NULL) {}
        ~AlgebraicDataType();
        
        // Getters
        std::string getName() { return name; }
        std::string *getKinds() { return kinds; }
        Type*** getArgss() { return argss; }

        bool isConstant(Tenv t);
        virtual bool depends_on_tvar(std::string, Tenv);

        Type* clone();
        Type* unify(Type*, Tenv);
        Type* simplify(Tenv);
        
        bool equals(Type*, Tenv);
        std::string toString();

};

/**
 * A base behavior for types that consist of exactly two types.
 */
class PairType : public Type {
    protected:
        Type *left;
        Type *right;
    public:
        PairType(Type *a, Type *b) : left(a), right(b) {}
        ~PairType() { delete left; delete right; }

        Type* getLeft() { return left; }
        Type* getRight() { return right; }

        bool isConstant(Tenv t) { return left->isConstant(t) && right->isConstant(t); }
        virtual bool depends_on_tvar(std::string x, Tenv tenv) { return left->depends_on_tvar(x, tenv) || right->depends_on_tvar(x, tenv); }
};

/**
 * A type that defines functions.
 * Denoted (s -> t)
 */
class LambdaType : public PairType {
    private:
        std::string id;
    public:
        LambdaType(std::string s, Type *a, Type *b)
            : PairType(a,b), id(s) {}
        LambdaType(Type *a, Type *b) : LambdaType("", a, b) {}

        Type* clone() { return new LambdaType(id, left->clone(), right->clone()); }
        Type* unify(Type*, Tenv);
        Type* simplify(Tenv tenv) { return new LambdaType(id, left->simplify(tenv), right->simplify(tenv)); }

        std::string getX() { return id; }

        bool equals(Type*, Tenv);
        std::string toString();
};

/**
 * A type that represents a list of a given type.
 * Denoted [t]
 */
class ListType : public Type {
    private:
        Type *type;
    public:
        ListType(Type *t) : type(t) {}
        ~ListType() { delete type; }
        Type* clone() { return new ListType(type->clone()); }
        Type* subtype() { return type; }

        Type* unify(Type*, Tenv);
        Type* simplify(Tenv tenv) { return new ListType(type->simplify(tenv)); }
        bool isConstant(Tenv t) { return type->isConstant(t); }

        bool equals(Type*, Tenv);
        std::string toString();

        bool depends_on_tvar(std::string x, Tenv tenv) { return type->depends_on_tvar(x, tenv); }
};

/**
 * Defines a pair type that consists of two types that represent a tuple.
 * Denoted (s * t)
 */
class TupleType : public PairType {
    public:
        using PairType::PairType;

        Type* clone() { return new TupleType(left->clone(), right->clone()); }
        Type* unify(Type*, Tenv);
        Type* simplify(Tenv tenv) { return new TupleType(left->simplify(tenv), right->simplify(tenv)); }

        bool equals(Type*, Tenv);
        std::string toString();
};

/**
 * An operator type that represents an arbitrary addition operation.
 * Denoted (s + t)
 */
class SumType : public PairType {
    public:
        using PairType::PairType;
        Type* clone() { return new SumType(left->clone(), right->clone()); }
        Type* unify(Type*, Tenv);
        Type* simplify(Tenv tenv);

        bool equals(Type*, Tenv);
        std::string toString();
};

/**
 * An operator type that represents an arbitrary addition operation.
 * Denoted (s x t)
 */
class MultType : public PairType {
    public:
        using PairType::PairType;
        Type* clone() { return new MultType(left->clone(), right->clone()); }
        Type* unify(Type*, Tenv);
        Type* simplify(Tenv tenv);

        bool equals(Type*, Tenv);
        std::string toString();
};

/**
 * An operator type that defines the differential operator.
 * Denoted ds/dt
 */
class DerivativeType : public PairType {
    public:
        using PairType::PairType;
        Type* clone() { return new DerivativeType(left->clone(), right->clone()); }
        Type* unify(Type*, Tenv);
        Type* simplify(Tenv);

        std::string toString();

};

// Primitive types
class PrimitiveType : public Type {};
class BoolType : public PrimitiveType {
    public:
        Type* clone() { return new BoolType; }
        Type* unify(Type*, Tenv);
        bool equals(Type*, Tenv);
        std::string toString() { return "B"; }
};
class RealType : public PrimitiveType {
    public:
        virtual Type* clone() { return new RealType; }
        virtual Type* unify(Type*, Tenv);
        bool equals(Type*, Tenv);
        virtual std::string toString() { return "R"; }
};
class IntType : public RealType {
    public:
        Type* clone() { return new IntType; }
        Type* unify(Type*, Tenv);
        bool equals(Type*, Tenv);
        std::string toString() { return "Z"; }
};
class StringType : public PrimitiveType {
    public:
        Type* clone() { return new StringType; }
        Type* unify(Type*, Tenv);
        bool equals(Type*, Tenv);
        std::string toString() { return "S"; }
};
class VoidType : public PrimitiveType {
    public:
        VoidType() {}
        Type* clone() { return new VoidType; }
        Type* unify(Type*, Tenv);
        bool equals(Type*, Tenv);
        std::string toString() { return "void"; }
};

// Dictionary type (records)
class DictType : public Type {
    private:
        HashMap<std::string, Type*> *types;
    public:
        DictType() { types = new HashMap<std::string, Type*>; }
        DictType(HashMap<std::string, Type*> *ts) : types(ts) {}
        DictType(std::initializer_list<std::pair<std::string, Type*>>);
        ~DictType() {
            auto it = types->iterator();
            while (it->hasNext()) delete types->get(it->next());
            delete it;
            delete types;
        }

        HashMap<std::string, Type*>* getTypes() { return types; }

        Type* clone();
        Type* unify(Type*, Tenv);
        Type* simplify(Tenv);

        bool equals(Type*, Tenv);
        bool depends_on_tvar(std::string, Tenv);

        std::string toString();

};

// Special types for polymorphism
class VarType : public Type {
    private:
        std::string name;
    public:
        VarType(std::string v) : name(v) {}
        Type* clone() { return new VarType(name); }
        Type* unify(Type*, Tenv);
        Type* simplify(Tenv tenv);
        bool isConstant(Tenv);
        bool equals(Type*, Tenv);
        bool depends_on_tvar(std::string, Tenv);

        std::string toString() { return name; }
};

#endif
