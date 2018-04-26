#include "types.hpp"
#include "expression.hpp"
#include "proof.hpp"

using namespace std;

Type* reduce_type(Type *t, Tenv tenv) {
    while (t && isType<VarType>(t)) {
        Type *v = tenv->get_tvar(t->toString());
        if (v->toString() == t->toString())
            return t;
        else
            t = v;
    }
    return t;
}

template<class T>
Type* reduces_to_type(Type *t, Tenv tenv) {
    t = reduce_type(t, tenv);
    if (isType<SumType>(t) || isType<MultType>(t)) {
        Type *a = reduces_to_type<T>(((PairType*) t)->getLeft(), tenv);
        if (a) return a;
        Type *b = reduces_to_type<T>(((PairType*) t)->getLeft(), tenv);
        if (b) return b;
        return NULL;
    } else
        return isType<VarType>(t) || isType<T>(t) ? t : NULL;
}

bool BoolType::equals(Type *t, Tenv tenv) {
    return reduces_to_type<BoolType>(t, tenv);
}

bool DictType::equals(Type *t, Tenv tenv) {
    t = reduces_to_type<DictType>(t, tenv);
    if (!t) return false;
    else if (isType<VarType>(t))
        return true;
    else if (isType<DictType>(t)) {
        auto T = (DictType*) t;
        auto it = types->iterator();
        
        // Equality checks the intersection of the dictionaries.
        while (it->hasNext()) {
            string k = it->next();
            if (T->types->hasKey(k)) {
                auto b = types->get(k)->equals(T->types->get(k), tenv);
                if (!b) {
                    delete it;
                    return false;
                }
            }
        }
        return true;
    } else
        return false;
}

bool IntType::equals(Type *t, Tenv tenv) {
    return reduces_to_type<RealType>(t, tenv);
}
bool RealType::equals(Type *t, Tenv tenv) {
    auto r = reduces_to_type<RealType>(t, tenv);
    auto z = reduces_to_type<IntType>(t, tenv);
    auto v = reduces_to_type<VarType>(t, tenv);

    return r && (!z || v);
}
bool StringType::equals(Type *t, Tenv tenv) {
    return reduces_to_type<StringType>(t, tenv);
}
bool VoidType::equals(Type *t, Tenv tenv) {
    return reduces_to_type<VoidType>(t, tenv);
}
bool LambdaType::equals(Type *t, Tenv tenv) {
    t = reduces_to_type<LambdaType>(t, tenv);
    if (!t)
        return false;
    else if (isType<VarType>(t))
        return true;
    else if (isType<LambdaType>(t)) {
        return left->equals(((LambdaType*) t)->left, tenv) && ((LambdaType*) t)->right->equals(right, tenv);
    } else
        return false;
}
bool ListType::equals(Type *t, Tenv tenv) {
    t = reduces_to_type<ListType>(t, tenv);
    if (!t)
        return false;
    else if (isType<VarType>(t))
        return true;
    else if (isType<ListType>(t))
        return type->equals(((ListType*) t)->type, tenv);
    else
        return false;
}
bool TupleType::equals(Type *t, Tenv tenv) {
    t = reduces_to_type<TupleType>(t, tenv);
    if (!t)
        return false;
    else if (isType<VarType>(t))
        return true;
    else if (isType<TupleType>(t))
        return left->equals(((TupleType*) t)->left, tenv) && right->equals(((TupleType*) t)->right, tenv);
    else
        return false;
}
bool VarType::equals(Type *t, Tenv tenv) {
    Type *a = reduce_type(this, tenv);
    Type *b = reduce_type(t, tenv);

    return isType<VarType>(a)
            || a->equals(b, tenv);
}
