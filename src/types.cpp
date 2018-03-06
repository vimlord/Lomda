#include "types.hpp"
#include "expression.hpp"

using namespace std;

template<typename T>
inline bool isType(const Type* t) {
    return t && dynamic_cast<const T*>(t) != nullptr;
}

string type_res_str(Tenv tenv, Exp exp, Type *type) {
    string env = tenv ? tenv->toString() : "{}";
    if (type)
        return env + " ⊢ " + exp->toString() + " : " + type->toString();
    else
        return env + " ⊢ " + exp->toString() + " is untypable";
}

void show_proof_step(string x) {
    std::cout << x << "\n";
}
void show_proof_therefore(string x) {
    show_proof_step("Therefore, " + x + ".");
}
void show_mgu_step(Tenv t, Type *a, Type *b, Type *c) {
    if (c)
        show_proof_step("Under " + t->toString() + ", " + a->toString() + " = " + b->toString() + " unifies to " + c->toString() + ".");
    else
        show_proof_step("Under " + t->toString() + ", " + a->toString() + " = " + b->toString() + " is not unifiable.");
}

TypeEnv::~TypeEnv() {
    for (auto it : types)
        delete it.second;

    for (auto it : mgu)
        delete it.second;
}

Type* TypeEnv::apply(string x) {
    if (types.find(x) == types.end()) {
        // We need to instantiate this type to be a new variable type
        auto V = make_tvar();
        types[x] = V;
    }
    return types[x]->clone();
}

bool TypeEnv::hasVar(string x) {
    return types.find(x) != types.end();
}

int TypeEnv::set(string x, Type* v) {
    int res = 0;
    if (types.find(x) != types.end()) {
        delete types[x];
        res = 1;
    }
    types[x] = v;
    return res;
}

Tenv TypeEnv::clone() {
    Tenv env = new TypeEnv;
    for (auto it : types)
        env->set(it.first, it.second->clone());
    for (auto it : mgu)
        env->set_tvar(it.first, it.second->clone());

    env->next_id = next_id;

    return env;
}

Type* TypeEnv::get_tvar(string v) {
    if (mgu.find(v) != mgu.end())
        return mgu[v];
    else
        return NULL;
}

void TypeEnv::set_tvar(string v, Type *t) {
    rem_tvar(v); mgu[v] = t;
}

void TypeEnv::rem_tvar(string v) {
    if (mgu.find(v) != mgu.end()) {
        delete mgu[v];
        mgu.erase(v);
    }
}

Type* TypeEnv::make_tvar() {
    auto V = new VarType(next_id);
    show_proof_step("Let " + next_id + " be a fresh type variable");

    // Increment the next_id var
    int i;
    for (i = next_id.length()-1; i >= 0 && next_id[i] == 'z'; i--)
        next_id[i] = 'a';
    if (i < 0)
        next_id = "a" + next_id;
    else
        next_id[i]++;

    mgu[V->toString()] = V;

    return V->clone();
}

bool VarType::isConstant(Tenv tenv) {
    auto T = tenv->get_tvar(name);
    if (isType<VarType>(T)) {
        if (T->toString() == name)
            return false;
        else
            return T->isConstant(tenv);
    } else
        return T->isConstant(tenv);
}

// Unification rules for fundamental type
Type* BoolType::unify(Type* t, Tenv tenv) {
    if (isType<BoolType>(t)) {
        show_mgu_step(tenv, this, t, this);
        return new BoolType;
    } else if (isType<PrimitiveType>(t)) {
        show_mgu_step(tenv, this, t, NULL);
        return NULL;
    } else
        return t->unify(this, tenv);
}
Type* IntType::unify(Type* t, Tenv tenv) {
    if (isType<RealType>(t)) {
        show_mgu_step(tenv, this, t, t);
        return t->clone();
    } else if (isType<PrimitiveType>(t)) {
        show_mgu_step(tenv, this, t, NULL);
        return NULL;
    } else
        return t->unify(this, tenv);
}
Type* LambdaType::unify(Type* t, Tenv tenv) {
    if (isType<LambdaType>(t)) {
        LambdaType *other = (LambdaType*) t;

        auto x = left->unify(other->left, tenv);
        if (!x) {
            show_proof_therefore("under " + tenv->toString() + ", "
                + toString() + " = " + other->toString()
                + " is not unifiable.");
            return NULL;
        }
        auto y = right->unify(other->right, tenv);
        if (!y) {
            show_proof_therefore("under " + tenv->toString() + ", "
                + toString() + " = " + other->toString()
                + " is not unifiable.");
            delete x;
            return NULL;
        }

        // We now know that the types are both x -> y. Hence, we will change
        // the types.

        show_proof_therefore("under " + tenv->toString() + ", "
                + toString() + " = " + other->toString()
                + " unifies to (" + x->toString() + " -> " + y->toString() + ").");

        delete other->left;
        delete other->right;
        other->left = x->clone();
        other->right = y->clone();

        delete left;
        delete right;
        left = x->clone();
        right = y->clone();


        return new LambdaType(x, y);
    } else
        return NULL;
}
Type* ListType::unify(Type* t, Tenv tenv) {
    if (isType<ListType>(t)) {
        auto A = type;
        auto B = ((ListType*) t)->type;
        auto C = type->unify(((ListType*) t)->type, tenv);

        if (C)
            show_proof_therefore("under " + tenv->toString() + ", " + toString() + " = " + t->toString() + " unifies to " + C->toString());
        else
            show_proof_therefore("under " + tenv->toString() + ", " + toString() + " = " + t->toString() + " is not unifiable");

        return C ? new ListType(C) : NULL;
    } else
        return t->unify(this, tenv);
}
Type* RealType::unify(Type* t, Tenv tenv) {
    if (isType<RealType>(t)) {
        show_mgu_step(tenv, this, t, this);
        return new RealType;
    } else if (isType<PrimitiveType>(t)) {
        show_mgu_step(tenv, this, t, NULL);
        return NULL;
    } else
        return t->unify(this, tenv);
}
Type* TupleType::unify(Type* t, Tenv tenv) {
    if (isType<TupleType>(t)) {
        TupleType *other = (TupleType*) t;

        auto x = left->unify(other->left, tenv);
        if (!x) {
            show_proof_therefore("under " + tenv->toString() + ", "
                + toString() + " = " + other->toString()
                + " is not unifiable");
            return NULL;
        }
        auto y = right->unify(other->right, tenv);
        if (!y) {
            show_proof_therefore("under " + tenv->toString() + ", "
                + toString() + " = " + other->toString()
                + " is not unifiable");
            delete x;
            return NULL;
        }

        show_proof_therefore("under " + tenv->toString() + ", "
                + toString() + " = " + other->toString()
                + " unifies to (" + x->toString() + " -> " + y->toString() + ")");
        
        // We now know that the types are both x * y. Hence, we will change
        // the types.
        delete other->left;
        delete other->right;
        other->left = x->clone();
        other->right = y->clone();

        delete left;
        delete right;
        left = x->clone();
        right = y->clone();

        return new TupleType(x, y);
    } else
        return NULL;
}
Type* VarType::unify(Type* t, Tenv tenv) {
    Type *A = tenv->get_tvar(name);

    if (isType<VarType>(t)) {
        // Acquire the other variable
        VarType *v = (VarType*) t;
        Type *B = tenv->get_tvar(v->name);

        show_proof_step("We will attempt to unify variables " + name + " and " + v->name);

        // Unify the two types
        Type* x;
        if (A->toString() != name || B->toString() != t->toString())
            x = A->unify(B, tenv);
        else
            x = clone();

        if (!x) {
            show_proof_therefore("under " + tenv->toString() + ", " + name + " = " + t->toString() + " is not unifiable");
            return NULL;
        }
        
        // Now, we need to update the variable values.
        tenv->set_tvar(v->name, x->clone());
        tenv->set_tvar(name, x->clone());

        return x;
    } else {
        show_proof_step("We will attempt to unify " + toString() + " = " + t->toString()
                + " by unifying " + A->toString() + " = " + t->toString() + ".");

        Type *T;
        // We must verify that this variable checks out
        if (A->toString() != name)
            T = A->unify(t, tenv);
        else
            // Since the variable identifies as itself, we can
            // define it to be whatever we want it to be.
            T = t->clone();
        
        // Show proof step
        if (T)
            show_proof_therefore("under " + tenv->toString() + ", " + T->toString() + "/" + name);
        else
            show_proof_therefore("under " + tenv->toString() + ", " + T->toString() + " = " + name + " is not unifiable");

        if (!T) return NULL;
        
        // Update the variable state
        tenv->set_tvar(name, T->clone());
         
        return T;
    }
}
Type* StringType::unify(Type* t, Tenv tenv) {
    if (isType<StringType>(t)) {
        show_mgu_step(tenv, this, t, this);
        return new StringType;
    } else if (isType<PrimitiveType>(t)) {
        show_mgu_step(tenv, this, t, NULL);
        return NULL;
    } else
        return t->unify(this, tenv);
}
Type* VoidType::unify(Type* t, Tenv tenv) {
    if (isType<VoidType>(t)) {
        show_mgu_step(tenv, this, t, this);
        return new VoidType;
    } else if (isType<PrimitiveType>(t)) {
        show_mgu_step(tenv, this, t, NULL);
        return NULL;
    } else
        return t->unify(this, tenv);
}

// Unification rules for operational types
Type* SumType::unify(Type* t, Tenv tenv) {
    if (isType<SumType>(t)) {
        SumType* other = (SumType*) t;

        auto x = left->unify(other->left, tenv);
        if (!x) return NULL;

        delete left;
        delete other->left;
        left = x;
        other->left = x->clone();

        auto y = right->unify(other->right, tenv);
        if (!y)
            return NULL;

        delete other->right;
        delete right;
        right = y;
        other->right = y->clone();;
        
        // Next, we can try unifying x and y
        auto z = x->unify(y, tenv);

        if (!z)
            // x and y have no unification. Hence, it is untypable.
            return NULL;
        else {
            delete x;
            delete y;
            left = z->clone();
            right = z->clone();
            other->left = z->clone();
            other->right = z->clone();
        }
        
        // We will now check to see if the result has been finalized; whether
        // or not this type is reducible to one of the two sides.
        x = z;
        while (isType<ListType>(x))
            x = ((ListType*) x)->subtype();

        if (isType<RealType>(x)) {
            // It has resolved to an nd array. Hence, the solution has been found.
            return z;
        } else if (isType<VarType>(x)) {
            delete other->right;
            other->right = z->clone();

            delete other->left;
            other->left = z->clone();

            delete right;
            other->right = z->clone();

            delete left;
            other->left = z->clone();
            
            return new SumType(z, z->clone());
        } else
            // There does not exist a unification
            return NULL;
    } else if (isType<RealType>(t)) {
        auto x = left->unify(t, tenv);
        if (!x) return NULL;

        delete left;
        left = x;
        
        x = right->unify(t, tenv);
        if (!x) return NULL;

        delete right;
        right = x;

        x = left->unify(right, tenv);
        if (!x) return NULL;

        delete left;
        delete right;

        left = x->clone();
        right = x->clone();

        auto z = x;
        while (isType<ListType>(x))
            x = ((ListType*) x)->subtype();

        if (isType<RealType>(x)) {
            // It has resolved to an nd array. Hence, the solution has been found.
            return z;
        } else if (isType<VarType>(x)) {
            return z;
        } else
            // There does not exist a unification
            return NULL;

    }
}

Type* MultType::unify(Type* t, Tenv tenv) {
    show_proof_step("Currently, typing of multiplication is undefined.");
    return NULL;
}


Type* LambdaExp::typeOf(Tenv tenv) {
    int i;
    for (i = 0; xs[i] != ""; i++);
    
    int argc = i;
    auto Ts = new Type*[argc];
    
    for (i = 0; i < argc; i++) {
        // Add it to the parameter arg list
        Ts[i] = tenv->make_tvar();
    }

    // We need to evaluate the type of the
    // body. This will also allow us to define
    // restrictions on the domain of the function.
    unordered_map<string,Type*> tmp;
    
    // Temporarily modify the environment
    for (i = 0; i < argc; i++) {
        if (tenv->hasVar(xs[i])) {
            auto t = tenv->apply(xs[i]);
            tmp[xs[i]] = t;
        }
        tenv->set(xs[i], Ts[i]->clone());
    }

    auto T = exp->typeOf(tenv);

    if (!T) {
        // The body could not be typed.
        while (argc--)
            delete Ts[argc];
        delete[] Ts;
        return NULL;
    } else if (isType<VarType>(T)) {
        // Simplify the body to be the value of the
        // variable, if it is known.
        auto V = tenv->get_tvar(T->toString())->clone();
        delete T;
        T = V;
    }
    if (argc == 0) {
        T = new LambdaType(new VoidType, T);
        ((LambdaType*) T)->setEnv(tenv->clone());;
    } else {
        auto env = tenv->clone();

        // Reset the environment
        for (int i = 0; i < argc; i++)
            // Remove the stuff
            tenv->remove(xs[i]);
        for (auto it : tmp)
            // Put the old stuff in
            tenv->set(it.first, it.second);

        for (i = argc - 1; i >= 0; i--) {
            T = new LambdaType(tenv->get_tvar(Ts[i]->toString())->clone(), T);
        }

        ((LambdaType*) T)->setEnv(env);
    }


    delete[] Ts;

    show_proof_therefore(type_res_str(tenv, this, T)); // QED

    return T;
}
Type* ApplyExp::typeOf(Tenv tenv) {
    auto T = op->typeOf(tenv);
    if (!isType<LambdaType>(T)) {
        show_proof_therefore(type_res_str(tenv, this, T));
        return NULL;
    }

    if (!args[0]) {
        // Function take zero arguments
        if (!isType<VoidType>(((LambdaType*) T)->getLeft())) {
            show_proof_therefore(type_res_str(tenv, this, NULL));
            delete T;
            return NULL;
        }
        
        show_proof_therefore(type_res_str(tenv, this, T));

        // Otherwise, it's the type of the right hand side.
        return ((LambdaType*) T)->getRight()->clone();
    }

    auto env = ((LambdaType*) T)->getEnv();
    if (env) env = env->clone();
    
    for (int i = 0; args[i]; i++) {
        if (!isType<LambdaType>(T)) {
            delete T;
            delete env;
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        }
        auto F = (LambdaType*) T;
        
        // Type the argument
        auto X = args[i]->typeOf(tenv);

        if (!X) {
            // The argument is untypable
            delete T;
            delete env;
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        }
        X = new LambdaType(X, env->make_tvar());

        // In the function tenv, we will unify the
        // argument types
        show_proof_step("To type " + toString() + ", we must unify " + X->toString() + " and " + F->toString() + ".");
        auto Z = X->unify(F, env);
        delete X;
        delete F;
        if (!Z) {
            // Non-unifiable
            delete env;
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        }
            
        // Continue
        T = ((LambdaType*) Z)->getRight()->clone();
        delete Z;
    }

    delete env;
    
    show_proof_therefore(type_res_str(tenv, this, T)); // QED

    // End case: return T
    return T;
}



Type* IfExp::typeOf(Tenv tenv) {
    auto C = cond->typeOf(tenv);
    if (!C) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto B = new BoolType;
    auto D = C->unify(B, tenv);
    delete B;
    delete C;

    if (!D) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    delete D;

    auto T = tExp->typeOf(tenv);
    if (!T) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto F = fExp->typeOf(tenv);
    if (!T) {
        delete T;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto Y = T->unify(F, tenv);
    delete T;
    delete F;

    show_proof_therefore(type_res_str(tenv, this, Y));

    return Y;
}
Type* IsaExp::typeOf(Tenv tenv) {
    auto T = exp->typeOf(tenv);
    if (!T) return NULL;
    delete T;

    return new BoolType;
}
Type* LetExp::typeOf(Tenv tenv) {
    unordered_map<string, Type*> tmp;
    
    int i;
    for (i = 0; exps[i]; i++) {
        show_proof_step("Let value " + ids[i] + " = " + exps[i]->toString()); // Initial condition.

        // We seek to define the type of the ith variable to be defined.
        auto t = exps[i]->typeOf(tenv);
    
        // Answer to typability
        show_proof_step("Therefore, " + tenv->toString() + " ⊢ " + ids[i] + (t ? (" : " + t->toString()) : " is untypable"));

        if (!t) {
            break;
        } else {
            if (tenv->hasVar(ids[i]))
                // We may have to suppress variables.
                tmp[ids[i]] = tenv->apply(ids[i]);

            tenv->set(ids[i], t);

        }
    }


    // Evaluate the body if possible
    auto T = exps[i] ? NULL : body->typeOf(tenv);

    show_proof_therefore(type_res_str(tenv, body, T)); // Early QED
    
    // Restore the tenv
    for (i = 0; exps[i]; i++)
        tenv->remove(ids[i]);
    for (auto it : tmp) {
        tenv->set(it.first, it.second);
        tmp[it.first] = NULL;
    }
    
    show_proof_therefore(type_res_str(tenv, this, T)); // QED

    return T;
}
Type* SequenceExp::typeOf(Tenv tenv) {
    Type *T = NULL;

    auto it = seq->iterator();
    do {
        if (T) delete T;
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
        S = T->unify(E, tenv);
        delete E;
    }
    
    delete T;
    
    return S;
}
Type* WhileExp::typeOf(Tenv tenv) {
    auto C = cond->typeOf(tenv);
    auto B = new BoolType;
    
    // Type the conditional
    auto D = C->unify(B, tenv);
    delete B;
    delete C;

    if (!D) return NULL;
    else delete D;
    
    // Type the body
    D = body->typeOf(tenv);
    if (!D) return NULL;
    else delete D;

    return new VoidType;
}

Type* AndExp::typeOf(Tenv tenv) {
    auto X = left->typeOf(tenv);
    if (!X) return NULL;

    auto Y = right->typeOf(tenv);
    if (!Y) {
        delete X;
        return NULL;
    }

    auto B = new BoolType;
    
    auto x = X->unify(B, tenv);
    delete X;
    if (!x) {
        delete Y;
        return NULL;
    }

    auto y = Y->unify(B, tenv);
    delete Y;
    if (!y) {
        delete x;
        return NULL;
    }
    
    delete x;
    delete y;

    return B;
}
Type* OrExp::typeOf(Tenv tenv) {
    auto X = left->typeOf(tenv);
    if (!X) return NULL;

    auto Y = right->typeOf(tenv);
    if (!Y) {
        delete X;
        return NULL;
    }

    auto B = new BoolType;
    
    auto x = X->unify(B, tenv);
    delete X;
    if (!x) {
        delete B;
        delete Y;
        return NULL;
    }

    auto y = Y->unify(B, tenv);
    delete Y;
    if (!y) {
        delete x;
        delete B;
        return NULL;
    }
    
    delete x;
    delete y;

    return B;
}
Type* NotExp::typeOf(Tenv tenv) {
    auto T = exp->typeOf(tenv);
    if (!T) return NULL;

    auto B = new BoolType;

    auto x = T->unify(B, tenv);
    delete T;
    if (!x) {
        delete B;
        B = NULL;
    }
    
    delete x;

    return B;

}

Type* ListExp::typeOf(Tenv tenv) {
    // If the list is empty, it could contain any type
    if (list->size() == 0)
        return new ListType(tenv->make_tvar());
    
    auto it = list->iterator();

    // Get the type of the first element
    auto T = it->next()->typeOf(tenv);
    
    // Next, we must verify that the type of each element is unifiable.
    while (T && it->hasNext()) {
        auto A = it->next()->typeOf(tenv);
        if (!A) {
            delete T;
            T = NULL;
            break;
        }

        auto B = T->unify(A, tenv);
        delete T;
        delete A;

        T = B;
    }

    delete it;
    
    return T ? new ListType(T) : NULL;

}
Type* ListAccessExp::typeOf(Tenv tenv) {
    auto L = list->typeOf(tenv);
    if (!L) return NULL;

    auto Ts = new ListType(tenv->make_tvar());

    auto A = L->unify(Ts, tenv);
    delete L;
    delete Ts;

    if (!A)
        return NULL;

    auto I = idx->typeOf(tenv);
    if (!I) {
        delete A;
        return NULL;
    }

    auto Z = new IntType;

    // The type of the index must be unifiable w/ Z
    auto B = I->unify(Z, tenv);
    delete I;
    delete Z;
    if (!B) {
        delete A;
        return NULL;
    }
    
    auto T = ((ListType*) A)->subtype()->clone();

    delete A;
    delete B;

    return T;
}
Type* ListSliceExp::typeOf(Tenv tenv) {
    auto L = list->typeOf(tenv);
    if (!L) return NULL;

    auto Ts = new ListType(tenv->make_tvar());
    
    auto A = L->unify(Ts, tenv);
    delete L;
    delete Ts;

    if (!A)
        return NULL;
    
    // Typing of the first index
    Type *I = NULL;
    Type *Z = NULL;
    
    if (from) {
        I = from->typeOf(tenv);
        if (!I) {
            delete A;
            return NULL;
        }

        Z = new IntType;

        // The type of the from index must be unifiable w/ Z
        auto B = I->unify(Z, tenv);
        delete I;
        delete Z;

        if (!B) {
            delete A;
            return NULL;
        } else delete B;
    }
    
    // Now, we type the second index if it is given.
    if (to) {
        I = to->typeOf(tenv);
        if (!I) {
            delete A;
            delete Z;
            return NULL;
        }
        Z = new IntType;
        auto B = I->unify(Z, tenv);
        delete I;
        delete Z;

        if (!B) {
            delete A;
            return NULL;
        }
    } else
        delete Z;

    return A;
}

// Typing rules that evaluate to type U + V
Type* SumExp::typeOf(Tenv tenv) {
    auto A = left->typeOf(tenv);
    if (!A) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto B = right->typeOf(tenv);
    if (!B) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        delete A;
        return NULL;
    }
    
    // We must unify the two types
    Type *C;
    if (A->isConstant(tenv) && B->isConstant(tenv)) {
        C = A->unify(B, tenv);
        delete A; delete B;
        show_proof_therefore(type_res_str(tenv, this, C));
        return C;
    } else {
        C = new SumType(A, B);
        show_proof_therefore(type_res_str(tenv, this, C));
    }
    return C;
}
Type* DiffExp::typeOf(Tenv tenv) {
    auto A = left->typeOf(tenv);
    if (!A) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto B = right->typeOf(tenv);
    if (!B) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        delete A;
        return NULL;
    }
    
    // We must unify the two types
    Type *C;
    if (A->isConstant(tenv) && B->isConstant(tenv)) {
        C = A->unify(B, tenv);
        delete A; delete B;
        show_proof_therefore(type_res_str(tenv, this, C));
        return C;
    } else {
        C = new SumType(A, B);
        show_proof_therefore(type_res_str(tenv, this, C));
    }
    return C;
}

Type* VarExp::typeOf(Tenv tenv) {
    auto T = tenv->apply(id);
    show_proof_step("We recognize " + id + " as being defined by " + T->toString());
    return T;
}



Type* IntExp::typeOf(Tenv tenv) {
    show_proof_step(toString() + " : Z trivially");
    return new IntType; }

Type* RealExp::typeOf(Tenv tenv) {
    show_proof_step(toString() + " : R trivially");
    return new RealType; }

Type* TrueExp::typeOf(Tenv tenv) {
    show_proof_step(toString() + " : B trivially");
    return new BoolType; }

Type* FalseExp::typeOf(Tenv tenv) {
    show_proof_step(toString() + " : B trivially");
    return new BoolType; }


Type* TupleExp::typeOf(Tenv tenv) {
    auto L = left->typeOf(tenv);
    if (!L) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto R = right->typeOf(tenv);
    if (!R) {
        delete L;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto T = new TupleType(L, R);
    show_proof_therefore(type_res_str(tenv, this, T));

    return T;
}
