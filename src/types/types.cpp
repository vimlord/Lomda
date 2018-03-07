#include "types.hpp"
#include "expression.hpp"
#include "proof.hpp"

using namespace std;

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
Type* MultType::subst(Tenv tenv) { return new MultType(left->subst(tenv), right->subst(tenv)); }

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
    show_proof_step("To type " + toString() + ", we must match the parameter type(s) of the function with its arguments.");
    auto T = op->typeOf(tenv);
    if (!isType<LambdaType>(T)) {
        show_proof_therefore(type_res_str(tenv, this, T));
        delete T;
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
        show_proof_step("To type " + toString() + ", we must unify " + X->toString() + " and " + F->toString() + " under " + env->toString() + ".");
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
        return C;
    } else {
        C = new SumType(A, B);
    }

    show_proof_therefore(type_res_str(tenv, this, C));

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
