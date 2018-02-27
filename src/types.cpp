#include "types.hpp"
#include "expression.hpp"

using namespace std;

template<typename T>
inline bool isType(const Type* t) {
    return t && dynamic_cast<const T*>(t) != nullptr;
}

string VarType::NEXT_ID = "a";
int TypeEnv::set(string x, Type* v) {
    v->add_ref();
    int res = 0;
    if (types.find(x) != types.end()) {
        types[x]->rem_ref();
        res = 1;
    }
    types[x] = v;
    return res;
}

Tenv TypeEnv::clone() {
    Tenv env = new TypeEnv;
    for (auto it : types) {
        env->set(it.first, it.second);
        it.second->add_ref();
    }
    return env;
}

string TypeEnv::toString() {
    string res = "{";
    int i = 0;
    for (auto it : types) {
        if (i) res += ", ";
        res += it.first + " := " + it.second->toString();
        i++;
    }
}

// Unification rules for fundamental type
Type* BoolType::unify(Type* t) {
    if (typeid(*t) == typeid(BoolType))
        return new BoolType;
    else if (isType<PrimitiveType>(t))
        return NULL;
    else
        return t->unify(this);
}
Type* IntType::unify(Type* t) {
    if (isType<RealType>(t)) {
        t->add_ref();
        return t;
    } else if (isType<PrimitiveType>(t))
        return NULL;
    else
        return t->unify(this);
}
Type* LambdaType::unify(Type* t) {
    if (typeid(*t) == typeid(LambdaType)) {
        LambdaType *other = (LambdaType*) t;

        auto x = left->unify(other->left);
        if (!x) return NULL;
        auto y = right->unify(other->right);
        if (!y) {
            x->rem_ref();
            return NULL;
        }

        // We now know that the types are both x -> y. Hence, we will change
        // the types.
        x->add_ref(); x->add_ref();
        y->add_ref(); y->add_ref();

        other->left->rem_ref(); other->left = x;
        other->right->rem_ref(); other->right = y;

        left->rem_ref(); left = x;
        right->rem_ref(); right = y;

        return new LambdaType(x, y);
    } else
        return NULL;
}
Type* ListType::unify(Type* t) {
    if (typeid(*t) != typeid(ListType))
        return NULL;
    else
        return type->unify(((ListType*) t)->type);
}
Type* RealType::unify(Type* t) {
    if (isType<RealType>(t))
        return new RealType;
    else if (isType<PrimitiveType>(t))
        return NULL;
    else
        return t->unify(this);
}
Type* TupleType::unify(Type* t) {
    if (typeid(*t) == typeid(TupleType)) {
        TupleType *other = (TupleType*) t;

        auto x = left->unify(other->left);
        if (!x) return NULL;
        auto y = right->unify(other->right);
        if (!y) {
            x->rem_ref();
            return NULL;
        }
        
        // We now know that the types are both x * y. Hence, we will change
        // the types.
        x->add_ref(); x->add_ref();
        y->add_ref(); y->add_ref();

        other->left->rem_ref(); other->left = x;
        other->right->rem_ref(); other->right = y;

        left->rem_ref(); left = x;
        right->rem_ref(); right = y;

        return new TupleType(x, y);
    } else
        return NULL;
}
Type* VarType::unify(Type* t) {
    if (typeid(*t) == typeid(VarType)) {
        VarType *v = (VarType*) t;
        v->name = name;

        if (type) {
            if (v->type) {
                // Unify the two types
                auto x = type->unify(v->type);
                if (!x) return NULL;

                type->rem_ref();
                v->type->rem_ref();
                x->add_ref();

                type = v->type = x;
            } else {
                // Assimilate the other type
                type->add_ref();
                v->type = type;
            }
        } else if (v->type) {
            // Assimilate the other type.
            v->type->add_ref();
            type = v->type;
        }

        add_ref();
        return this;
    }
}
Type* StringType::unify(Type* t) {
    if (typeid(*t) == typeid(StringType))
        return new VoidType;
    else if (isType<PrimitiveType>(t))
        return NULL;
    else
        return t->unify(this);
}
Type* VoidType::unify(Type* t) {
    if (typeid(*t) == typeid(VoidType))
        return new VoidType;
    else if (isType<PrimitiveType>(t))
        return NULL;
    else
        return t->unify(this);
}

// Unification rules for operational types
Type* SumType::unify(Type* t) {
    if (typeid(*t) == typeid(SumType)) {
        SumType* other = (SumType*) t;

        auto x = left->unify(other->left);
        if (!x) return NULL;
        left->rem_ref();
        left = x;
        other->left->rem_ref(); other->left = x; other->left->add_ref();

        auto y = right->unify(other->right);
        if (!y)
            return NULL;
        right->rem_ref();
        right = y;
        other->right->rem_ref(); other->right = y; other->right->add_ref();
        
        // Next, we can try unifying x and y
        auto z = x->unify(y);

        if (!z)
            // x and y have no unification. Hence, it is untypable.
            return NULL;
        else {
            for (int i = 0; i < 4; i++) z->add_ref();
            x->rem_ref(); x->rem_ref();
            y->rem_ref(); y->rem_ref();
            left = right = other->left = other->right = z;
        }
        
        // We will now check to see if the result has been finalized; whether
        // or not this type is reducible to one of the two sides.
        x = z;
        while (typeid(*x) == typeid(ListType))
            x = ((ListType*) x)->subtype();

        if (isType<RealType>(x)) {
            // It has resolved to an nd array. Hence, the solution has been found.
            return z;
        } else if (isType<VarType>(x)) {
            
            // The type is x + y. We can update our type.
            for (int i = 0; i < 4; i++)
                z->add_ref();

            other->right->rem_ref(); other->right = z;

            left->rem_ref(); left = z;
            right->rem_ref(); right = z;
            
            return new SumType(x, y);
        } else
            // There does not exist a unification
            return NULL;
    } else if (isType<RealType>(t)) {
        auto x = left->unify(t);
        if (!x) return NULL;
        left->rem_ref();
        left = x;
        
        x = right->unify(t);
        if (!x) return NULL;
        right->rem_ref();
        right = x;

        x = left->unify(right);
        if (!x) return NULL;
        x->add_ref();
        left->rem_ref(); left = x;
        right->rem_ref(); right = x;

        auto z = x;
        while (typeid(*x) == typeid(ListType))
            x = ((ListType*) x)->subtype();

        if (isType<RealType>(x)) {
            // It has resolved to an nd array. Hence, the solution has been found.
            return z;
        } else if (isType<VarType>(x)) {
            add_ref();
            return this;
        } else
            // There does not exist a unification
            return NULL;

    }
}

Type* MultType::unify(Type* t) {
    return NULL;
}

// Lambda typing rules
Type* LambdaExp::typeOf(Tenv tenv) {
    if (xs[0] == "") {
        Type *T = exp->typeOf(tenv);
        if (T)
            return new LambdaType(new VoidType, T, tenv->clone());
        else
            return NULL;
    } else {
        int i;
        for (i = 0; xs[i] != ""; i++);
        int len = i;
        
        auto Ts = new Type*[i+1];
        auto Es = new Tenv[i+1];
        Ts[i] = NULL;
        Es[i] = NULL;
        while (i--)
            Ts[i] = new VarType;
        while (Ts[++i]) {
            tenv = Es[i] = tenv->clone();
            tenv->set(xs[i], Ts[i]);
        }

        auto T = exp->typeOf(tenv);
        if (!T) {
            while (i--) {
                Ts[i]->rem_ref();
                Es[i]->rem_ref();
            }
            delete Ts;
            delete Es;
            return NULL;
        }
        while (i--)
            T = new LambdaType(Ts[i], T, Es[i]);

        delete Ts, Es;

        return T;
    }
}

Type* IfExp::typeOf(Tenv tenv) {
    auto C = cond->typeOf(tenv);
    if (!C) return NULL;

    auto B = new BoolType;
    auto D = C->unify(B);
    B->rem_ref();
    C->rem_ref();

    if (!D) return NULL;

    auto T = tExp->typeOf(tenv);
    auto F = fExp->typeOf(tenv);

    auto Y = T->unify(F);
    T->rem_ref();
    F->rem_ref();

    return Y;
}
Type* IsaExp::typeOf(Tenv tenv) {
    auto T = exp->typeOf(tenv);
    if (!T) return NULL;
    else T->rem_ref();

    return new BoolType;
}
Type* LetExp::typeOf(Tenv tenv) {
    unordered_map<string, Type*> tmp;
    
    int i;
    for (i = 0; exps[i]; i++) {
        auto t = exps[i]->typeOf(tenv);
        if (!t) {
            break;
        } else {
            auto s = tenv->apply(ids[i]);
            if (s) {
                // We may have to suppress variables.
                s->add_ref();
                tmp[ids[i]] = s;
            }
            tenv->set(ids[i], t);
        }
    }

    // Evaluate the body if possible
    auto T = exps[i] ? NULL : body->typeOf(tenv);
    
    // Restore the tenv
    for (auto it : tmp) {
        tenv->set(it.first, it.second);
        tmp.erase(it.first);
    }

    return T;
}
Type* SequenceExp::typeOf(Tenv tenv) {
    Type *T = NULL;

    auto it = seq->iterator();
    do {
        if (T) T->rem_ref();
        T = it->next()->typeOf(tenv);
    } while (it->hasNext() && T);

    delete it;
    return T;
}
Type* SetExp::typeOf(Tenv tenv) {
    // The target and expression should each have a type
    auto T = tgt->typeOf(tenv);
    if (!T) return NULL;
    auto E = exp->typeOf(tenv);

    Type *S = NULL;
    
    if (E) {
        // The type of the expression is the unification of
        // the two expressions
        S = T->unify(E);
        E->rem_ref();
    }

    T->rem_ref();
    
    return S;
}
Type* WhileExp::typeOf(Tenv tenv) {
    auto C = cond->typeOf(tenv);
    auto B = new BoolType;
    
    // Type the conditional
    auto D = C->unify(B);
    C->rem_ref();
    B->rem_ref();
    if (!D) return NULL;
    else D->rem_ref();
    
    // Type the body
    D = body->typeOf(tenv);
    if (!D) return NULL;
    else D->rem_ref();

    return new VoidType;
}

Type* AndExp::typeOf(Tenv tenv) {
    auto X = left->typeOf(tenv);
    if (!X) return NULL;

    auto Y = right->typeOf(tenv);
    if (!Y) {
        X->rem_ref();
        return NULL;
    }

    auto B = new BoolType;
    
    auto x = X->unify(B);
    X->rem_ref();
    if (!x) {
        Y->rem_ref();
        return NULL;
    }

    auto y = Y->unify(B);
    Y->rem_ref();
    if (!y) {
        x->rem_ref();
        return NULL;
    }
    
    x->rem_ref();
    y->rem_ref();

    return B;
}
Type* OrExp::typeOf(Tenv tenv) {
    auto X = left->typeOf(tenv);
    if (!X) return NULL;

    auto Y = right->typeOf(tenv);
    if (!Y) {
        X->rem_ref();
        return NULL;
    }

    auto B = new BoolType;
    
    auto x = X->unify(B);
    X->rem_ref();
    if (!x) {
        Y->rem_ref();
        B->rem_ref();
        return NULL;
    }

    auto y = Y->unify(B);
    Y->rem_ref();
    if (!y) {
        x->rem_ref();
        B->rem_ref();
        return NULL;
    }
    
    x->rem_ref();
    y->rem_ref();

    return B;
}
Type* NotExp::typeOf(Tenv tenv) {
    auto T = exp->typeOf(tenv);
    if (!T) return NULL;

    auto B = new BoolType;

    auto x = T->unify(B);
    T->rem_ref();
    if (!x) {
        B->rem_ref();
        B = NULL;
    }

    x->rem_ref();

    return B;

}

// Typing rules that evaluate to type U + V
Type* SumExp::typeOf(Tenv tenv) {
    auto A = left->typeOf(tenv);
    if (!A) return NULL;

    auto B = right->typeOf(tenv);
    if (!B) {
        A->rem_ref();
        return NULL;
    }
    
    // We must unify the two types
    auto C = A->unify(B);

    if (!C) return NULL;
    else if (isType<PrimitiveType>(C))
        return C;
    else {
        C->add_ref();
        return new SumType(C, C);
    }
}
Type* DiffExp::typeOf(Tenv tenv) {
    auto A = left->typeOf(tenv);
    if (!A) return NULL;

    auto B = right->typeOf(tenv);
    if (!B) {
        A->rem_ref();
        return NULL;
    }
    
    // We must unify the two types
    auto C = A->unify(B);

    if (!C) return NULL;
    else if (isType<PrimitiveType>(C))
        return C;
    else {
        C->add_ref();
        return new SumType(C, C);
    }
}

Type* VarExp::typeOf(Tenv tenv) {
    auto T = tenv->apply(id);
    if (!T) {
        // We will create a variable to represent the variable
        tenv->set(id, T = new VarType);
    } else
        T->add_ref();
    return T;
}


