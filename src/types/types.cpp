#include "types.hpp"
#include "expression.hpp"
#include "proof.hpp"

using namespace std;

Type* reduce_type(Type *t, Tenv tenv) {
    while (tenv && isType<VarType>(t)) {
        Type *v = tenv->get_tvar(t->toString());
        if (v->toString() == t->toString())
            return t;
        else
            t = v;
    }
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
        return isType<T>(t) ? t : NULL;
}

bool BoolType::equals(Type *t, Tenv tenv) {
    return reduces_to_type<BoolType>(t, tenv);
}
bool IntType::equals(Type *t, Tenv tenv) {
    return reduces_to_type<IntType>(t, tenv);
}
bool RealType::equals(Type *t, Tenv tenv) {
    return reduces_to_type<RealType>(t, tenv);
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
    else if (isType<LambdaType>(t))
        return left->equals(((LambdaType*) t)->left, tenv) && right->equals(((LambdaType*) t)->right, tenv);
    else
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
bool MultType::equals(Type *t, Tenv tenv) {
    return left->equals(t, tenv) || right->equals(t, tenv);
}
bool SumType::equals(Type *t, Tenv tenv) {
    return left->equals(t, tenv) || right->equals(t, tenv);
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

Type* VarType::subst(Tenv tenv) {
    // Get the known value
    Type *T = tenv->get_tvar(name);

    // If we have some idea, we can simplify.
    if (T->toString() != name) {
        T = T->subst(tenv);
    
        // Set the new variable
        tenv->set_tvar(name, T);
    }
    
    // Return a copy
    return T->clone();
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
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto y = Y->unify(B, tenv);
    delete Y;
    if (!y) {
        delete x;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    delete x;
    delete y;

    show_proof_therefore(type_res_str(tenv, this, B));

    return B;
}
Type* ApplyExp::typeOf(Tenv tenv) {
    show_proof_step("To type " + toString() + ", we must match the parameter type(s) of the function with its arguments.");
    auto T = op->typeOf(tenv);

    auto Tmp = T->subst(tenv);
    delete T;
    T = Tmp;

    show_proof_step("Thus, the function " + op->toString() + " in function call " + toString() + " has type " + T->toString() + " under " + tenv->toString() + ".");

    if (isType<VarType>(T)) {
        // We need to create a unification that indicates that
        // the variable is a function.
        
        if (!args[0]) {
            // The function should type as a void -> t
            auto Y = tenv->make_tvar();
            auto F = new LambdaType(new VoidType, Y);
            F->setEnv(tenv->clone());
            auto G = T->unify(F, tenv);
            delete T;
            delete F;
            delete G;

            return Y;
        }
        
        int argc;
        for (argc = 0; args[argc]; argc++);

        Type **xs = new Type*[argc];
        for (int i = 0; args[i]; i++) {
            auto X = args[i]->typeOf(tenv);
            if (!X) {
                while (i--) delete xs[i];
                delete[] xs;
                show_proof_therefore(type_res_str(tenv, this, NULL));
                return NULL;
            } else {
                show_proof_step("Knowing " + type_res_str(tenv, args[i], X) + ", we are closer to inferring the type of " + op->toString() + ".");
                xs[i] = X;
            }
        }
        
        auto Y = tenv->make_tvar();
        
        // Build the lambda type, starting from some arbitrary
        // body type that could be decided later.
        auto F = Y->clone();
        show_proof_step("We will use " + F->toString() + " to define the currently arbitrary return type of " + op->toString() + ", which takes " + to_string(argc) + " arguments.");
        while (argc--) {
            show_proof_step("We know that " + type_res_str(tenv, args[argc], xs[argc]) + ".");
            F = new LambdaType(xs[argc], F);
        }
        ((LambdaType*) F)->setEnv(tenv->clone());
        
        // Unification of the function type
        auto S = T->unify(F, tenv);
        
        // GC
        delete T;
        delete F;
        delete[] xs;

        if (!S) {
            delete Y;
            Y = NULL;
        }
         
        show_proof_therefore(type_res_str(tenv, this, Y));
        return Y;

    } else if (isType<LambdaType>(T)) {
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

        auto env = ((LambdaType*) T)->getEnv()->clone();

        show_proof_step("We must consolidate " + T->toString() + " under " + tenv->toString() + ".");
        
        for (int i = 0; args[i]; i++) {
            if (!isType<LambdaType>(T)) {
                show_proof_therefore(type_res_str(tenv, this, NULL));
                delete T;
                delete env;
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
            show_proof_step("To type " + toString() + ", we must unify " + X->toString() + " = " + F->toString() + " under " + env->toString() + ".");
            auto Z = X->unify(F, env);
            delete F;
            delete X;
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
    } else {
        show_proof_therefore(type_res_str(tenv, this, T));
        delete T;
        return NULL;
    }
}
Type* CastExp::typeOf(Tenv tenv) {
    auto T = exp->typeOf(tenv);
    if (!T) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    delete T;

    T = new BoolType;
    show_proof_therefore(type_res_str(tenv, this, T));
    return T;
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
        return C;
    } else {
        C = new SumType(A, B);
    }

    show_proof_therefore(type_res_str(tenv, this, C));

    return C;
}
Type* FoldExp::typeOf(Tenv tenv) {
    /* Typing requires the following:
    list : [a]
    base : b
    func : b -> a -> b
    */

    Type *L = list->typeOf(tenv);
    if (!L) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    Type *a;
    if (isType<ListType>(L))
        a = ((ListType*) L)->subtype()->clone();
    else if (isType<VarType>(L)) {
        auto M = new ListType(tenv->make_tvar());
        auto N = L->unify(M, tenv);
        delete M;
        if (N) {
            a = ((ListType*) N)->subtype()->clone();
            delete N;
        } else
            a = NULL;
    } else
        a = NULL;

    delete L;
    if (!a) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    Type *b = base->typeOf(tenv);
    if (!b) {
        delete a;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    Type *F = func->typeOf(tenv);
    if (!F) {
        delete a;
        delete b;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    // The function must be properly unifiable
    Type *G = new LambdaType(b->clone(), new LambdaType(a, b->clone()));
    Type *H = F->unify(G, tenv);
    
    delete F;
    delete G;

    if (H) {
        delete H;
        auto T = b->subst(tenv);
        delete b;
        b = T;
    } else {
        delete b;
        b = NULL;
    }

    show_proof_therefore(type_res_str(tenv, this, b));

    return b;
}
Type* ForExp::typeOf(Tenv tenv) {
    auto T = set->typeOf(tenv);
    if (!T) return NULL;

    auto L = new ListType(tenv->make_tvar());
    
    auto S = T->unify(L, tenv);
    delete T;
    delete L;

    if (!S) return NULL;
    S = ((ListType*) S)->subtype();
    
    show_proof_step("Thus, in the for body " + body->toString() + ", " + id + " : " + S->toString() + ".");
    show_proof_step("Let " + id + " : " + S->toString() + ".");
    
    // Remove the variable name from scope.
    Type *X = tenv->hasVar(id)
        ? tenv->apply(id)
        : NULL;

    // Add the var
    tenv->set(id, S);
    
    // Evaluate the body type
    T = body->typeOf(tenv);

    // Clean out the types
    if (X)
        tenv->set(id, X);
    else
        tenv->remove(id);
    
    // We now know the answer
    if (T) {
        delete T;
        T = new VoidType;
    }

    show_proof_therefore(type_res_str(tenv, this, T));
    return T;
}
Type* HasExp::typeOf(Tenv tenv) {
    auto X = item->typeOf(tenv);
    if (!X) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto Xs = set->typeOf(tenv);
    if (!Xs) {
        delete X;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    if (isType<ListType>(Xs)) {
        auto Y = X->unify(((ListType*) Xs)->subtype(), tenv);
        delete X;
        delete Xs;
        if (Y) {
            X = new BoolType;
            delete Y;
        } else
            X = NULL;
    } else if (isType<VarType>(Xs)) {
        auto V = (VarType*) X;
        auto L = new ListType(X);
        X = Xs->unify(L, tenv);
        delete L;
        delete V;
        if (X) {
            delete X;
            X = new BoolType;
        } else
            X = NULL;
    } else {
        delete Xs;
        delete X;
        X = NULL;
    }

    show_proof_therefore(type_res_str(tenv, this, X));
    return X;
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
    if (!T) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    delete T;

    T = new BoolType;
    show_proof_therefore(type_res_str(tenv, this, T));
    return T;
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
    
    // Type the body
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
        show_proof_step("Hence, the type of body expression " + exp->toString() + " is that of variable " + T->toString() + " = " + V->toString() + " under " + tenv->toString() + ".");
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
    
    if (T) {
        T = new ListType(T);
        show_proof_therefore(type_res_str(tenv, this, T));
        return T;
    } else {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    

}
Type* ListAccessExp::typeOf(Tenv tenv) {
    auto L = list->typeOf(tenv);
    if (!L) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto Ts = new ListType(tenv->make_tvar());

    auto A = L->unify(Ts, tenv);
    delete L;
    delete Ts;

    if (!A) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto I = idx->typeOf(tenv);
    if (!I) {
        delete A;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto Z = new IntType;

    // The type of the index must be unifiable w/ Z
    auto B = I->unify(Z, tenv);
    delete I;
    delete Z;
    if (!B) {
        delete A;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    auto T = ((ListType*) A)->subtype()->clone();

    delete A;
    delete B;

    return T;
}
Type* ListSliceExp::typeOf(Tenv tenv) {
    auto L = list->typeOf(tenv);
    if (!L) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

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
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        }

        Z = new IntType;

        // The type of the from index must be unifiable w/ Z
        auto B = I->unify(Z, tenv);
        delete I;
        delete Z;

        if (!B) {
            delete A;
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        } else delete B;
    }
    
    // Now, we type the second index if it is given.
    if (to) {
        I = to->typeOf(tenv);
        if (!I) {
            delete A;
            delete Z;
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        }
        Z = new IntType;
        auto B = I->unify(Z, tenv);
        delete I;
        delete Z;

        if (!B) {
            delete A;
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        }
    } else
        delete Z;

    show_proof_therefore(type_res_str(tenv, this, A));
    return A;
}
Type* MagnitudeExp::typeOf(Tenv tenv) {
    auto T = exp->typeOf(tenv);
    
    Type *Y;
    if (T) {
        if (isType<IntType>(T)
        ||  isType<ListType>(T)
        ||  isType<StringType>(T)
        ||  isType<BoolType>(T)
        )
            Y = new IntType;
        else if (isType<RealType>(T)
             ||  isType<VarType>(T))
            Y = new RealType;
        else
            Y = NULL;
        
        delete T;
    } else
        Y = NULL;

    show_proof_therefore(type_res_str(tenv, this, Y));
    
    return Y;
}
Type* MapExp::typeOf(Tenv tenv) {
    /* To type a map, we require that:
    1) list : [a] for some t
    2) func : a -> b
    Then, map : [b]
    */
    Type *F = func->typeOf(tenv);
    if (!F) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    Type *L = list->typeOf(tenv);
    if (!L) {
        delete F;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    show_proof_step("To type the map, we let the function have type "
            + F->toString() + " and the list have type " + L->toString() + ".");
    
    // Compute the type of the source type
    Type *a = NULL;
    if (isType<ListType>(L)) {
        a = ((ListType*) L)->subtype()->clone();
    } else if (isType<VarType>(L)) {
        Type *t = new ListType(tenv->make_tvar());
        Type *u = L->unify(t, tenv);
        delete t;

        if (u) {
            a = ((ListType*) u)->subtype()->clone();
            delete u;
        }
    }
    
    // If we can't find a, then we're screwed.
    delete L;
    if (!a) {
        delete F;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    // Next, we need to verify the functionness of F, and
    // go from there.
    Type *t = new LambdaType(a->clone(), tenv->make_tvar());
    show_proof_step("We require that " + F->toString() + " and " + t->toString() + " unify under " + tenv->toString() + ".");
    Type *u = F->unify(t, tenv);
    delete t;
    
    Type *b = NULL;
    if (u) {
        b = ((LambdaType*) u)->getRight()->clone();
        delete u;
    }

    delete F;
    delete a;
    
    // Final simplifications
    if (b) {
        a = b->subst(tenv);
        delete b;
        b = a;
    }

    L = b ? new ListType(b) : NULL;

    show_proof_therefore(type_res_str(tenv, this, L));

    return L;
}
Type* MultType::subst(Tenv tenv) { return new MultType(left->subst(tenv), right->subst(tenv)); }
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
    
    show_proof_therefore(type_res_str(tenv, this, B));

    return B;

}
Type* OrExp::typeOf(Tenv tenv) {
    auto X = left->typeOf(tenv);
    if (!X) return NULL;

    auto Y = right->typeOf(tenv);
    if (!Y) {
        delete X;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto B = new BoolType;
    
    auto x = X->unify(B, tenv);
    delete X;
    if (!x) {
        delete B;
        delete Y;
        show_proof_therefore(type_res_str(tenv, this, NULL));
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

    show_proof_therefore(type_res_str(tenv, this, B));

    return B;
}
Type* PrintExp::typeOf(Tenv tenv) {
    for (int i = 0; args[i]; i++) {
        Type *T = args[i]->typeOf(tenv);
        if (!T) {
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        } else
            delete T;
    }
    
    Type *T = new VoidType;
    show_proof_therefore(type_res_str(tenv, this, T));
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

    show_proof_therefore(type_res_str(tenv, this, T));

    return T;
}
Type* SetExp::typeOf(Tenv tenv) {
    // The target and expression should each have a type
    auto T = tgt->typeOf(tenv);
    if (!T) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    auto E = exp->typeOf(tenv);

    Type *S = NULL;
    
    if (E) {
        // The type of the expression is the unification of
        // the two expressions
        S = T->unify(E, tenv);
        delete E;
    }
    
    delete T;
    
    show_proof_therefore(type_res_str(tenv, this, S));
    
    return S;
}
Type* SumType::subst(Tenv tenv) {
    // Simplify both sides
    auto L = left->subst(tenv);
    auto R = right->subst(tenv);
    delete left; delete right;
    left = L; right = R;
    
    // Unify if possible
    if (L->isConstant(tenv) && R->isConstant(tenv)) {
        auto S = L->unify(R, tenv);
        return S;
    } else
        return clone();
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
Type* TupleAccessExp::typeOf(Tenv tenv) {
    auto T = exp->typeOf(tenv);
    if (T) {
    if (isType<TupleType>(T)) {
        auto TT = (TupleType*) T;
        T = (idx ? TT->getRight() : TT->getLeft())->clone();
        delete TT;
    } else if (isType<VarType>(T)) {
        // Simplify and toss out 
        auto TT = new TupleType(tenv->make_tvar(), tenv->make_tvar());
        auto t = T->unify(TT, tenv);
        delete T;
        delete TT;

        if (t) {
            T = (idx ?
                ((TupleType*) t)->getRight()
                : ((TupleType*) t)->getLeft())->clone();
            delete t;
        } else
            T = NULL;
    } else {
        delete T;
        T = NULL;
    }
    }
    
    // End result
    show_proof_therefore(type_res_str(tenv, this, T));
    return T;
}
Type* WhileExp::typeOf(Tenv tenv) {
    auto C = cond->typeOf(tenv);
    auto B = new BoolType;
    
    // Type the conditional
    auto D = C->unify(B, tenv);
    delete B;
    delete C;

    if (!D) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    else delete D;
    
    // Type the body
    D = body->typeOf(tenv);
    if (!D) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    else delete D;

    D = new VoidType;
    show_proof_therefore(type_res_str(tenv, this, D));
    return D;
}


// Typing of primitives
Type* VarExp::typeOf(Tenv tenv) {
    auto T = tenv->apply(id);
    show_proof_step("We recognize " + id + " as being defined by " + T->toString() + ".");
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
