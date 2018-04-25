#include "types.hpp"
#include "expression.hpp"
#include "proof.hpp"

#include "bnf.hpp"
#include <fstream>

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

Type* DictType::clone() {
    auto ts = new Trie<Type*>;

    auto tt = types->iterator();
    while (tt->hasNext()) {
        string k = tt->next();
        ts->add(k, types->get(k));
    }
    
    return new DictType(ts);
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
bool BoolType::equals(Type *t, Tenv tenv) {
    return reduces_to_type<BoolType>(t, tenv);
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

Type* MultType::subst(Tenv tenv) {
    auto L = left->subst(tenv);
    auto R = right->subst(tenv);
    delete left; delete right;
    left = L; right = R;
    
    if (L->isConstant(tenv) && R->isConstant(tenv)) {
        int a = 0;
        while (isType<ListType>(L)) {
            L = ((ListType*) L)->subtype();
            a++;
        }
        
        int b = 0;
        while (isType<ListType>(R)) {
            R = ((ListType*) R)->subtype();
            b++;
        }
        
        if (a > 2)
            // Will evaluate to bad type
            return NULL;
        else if (b > 2)
            // Will evaluate to bad type
            return NULL;
        
        auto T = L->unify(R, tenv); 

        if (!T)
            return NULL;
        else if (isType<VarType>(T)) {
            // We can only assume that T is of the real type
            RealType R;
            auto U = T->unify(&R, tenv);
            delete T;
            T = U;
        } else if (!(isType<RealType>(T) || isType<IntType>(T))) {
            delete T;
            return NULL;
        }
        
        // The outcome is based on the combined orders.
        switch(a+b) {
            case 4:
                return new ListType(new ListType(T));
            case 3:
                return new ListType(T);
            case 2:
                return a == b ? T : new ListType(new ListType(T));
            case 1:
                return new ListType(T);
            default:
                return T;
        }
    } else
        return clone();
    
}

Type* DerivativeType::subst(Tenv tenv) {
    auto L = left->subst(tenv);
    auto R = right->subst(tenv);
    delete left; delete right;
    left = L; right = R;

    if (isType<ListType>(L)) {
        R = new DerivativeType(((ListType*) L)->subtype()->clone(), R->clone());
        L = R->subst(tenv);
        delete R;

        if (L) return new ListType(L);
        else return NULL;
        
    } else if (isType<RealType>(L) || isType<IntType>(L)) {
        if (isType<RealType>(R) || isType<IntType>(R))
            return L->clone();
        else if (isType<ListType>(R)) {
            R = new DerivativeType(L->clone(), ((ListType*) R)->subtype()->clone());
            L = R->subst(tenv);
            delete R;

            if (L) return new ListType(L);
            else return NULL;
        } else if (isType<TupleType>(R)) {
            R = new TupleType(
                new DerivativeType(L->clone(), ((TupleType*) R)->getLeft()->clone()),
                new DerivativeType(L->clone(), ((TupleType*) R)->getRight()->clone()));

            L = R->subst(tenv);
            delete R;
            
            return L;
        } else if (isType<VarType>(R))
            return clone();
        else
            return NULL;

        return R->clone();
    } else if (isType<TupleType>(L)) {
        R = new TupleType(
            new DerivativeType(((TupleType*) L)->getLeft()->clone(), R->clone()),
            new DerivativeType(((TupleType*) L)->getRight()->clone(), R->clone()));

        L = R->subst(tenv);
        delete R;
        
        return L;
    } else if (isType<VarType>(L))
        return clone();
    else if (isType<LambdaType>(L)) {
        auto F = (LambdaType*) L;

        // The input type remains the same
        auto X = F->getLeft()->clone();

        // Derive the output of the lambda wrt the input type of derivative
        R = new DerivativeType(F->getRight()->clone(), R->clone());
        auto Y = R->subst(tenv);
        delete R;
         
        // Check the validity of the substitution generated.
        if (!Y) return NULL;
        
        // d(x->y)/dz = x->(dy/dz)
        return new LambdaType(F->getX(), X, Y);
    } else if (isType<SumType>(L)) {
        SumType *S = (SumType*) L;
        auto T = new SumType(
                new DerivativeType(S->getLeft()->clone(), R->clone()),
                new DerivativeType(S->getRight()->clone(), R->clone())
        );
        auto U = T->subst(tenv);
        delete T;
        
        return U;
    } else if (isType<SumType>(L)) {
        MultType *S = (MultType*) L;
         
        Type *d = new DerivativeType(S->getLeft()->clone(), R->clone());
        auto dL = d->subst(tenv);
        delete d;

        d = new DerivativeType(S->getRight()->clone(), R->clone());
        auto dR = d->subst(tenv);
        delete d;

        auto T = new SumType(
            new MultType(L->clone(), dR),
            new MultType(R->clone(), dL)
        );

        auto U = T->subst(tenv);
        delete T;
        
        return U;
    } else
        return NULL;
}

Type* SumType::subst(Tenv tenv) {
    // Simplify both sides
    auto L = left->subst(tenv);
    auto R = right->subst(tenv);
    delete left; delete right;
    left = L; right = R;
    
    if (L->isConstant(tenv) && R->isConstant(tenv)) {
        // We will unify if the two sides are constant
        auto S = L->unify(R, tenv);
        return S;
    } else
        return clone();
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
            auto F = new LambdaType("", new VoidType, Y);
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
            F = new LambdaType("", xs[argc], F);
        }
        
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

        show_proof_step("We must consolidate " + T->toString() + " under " + tenv->toString() + ".");
        
        for (int i = 0; args[i]; i++) {
            if (!isType<LambdaType>(T)) {
                show_proof_therefore(type_res_str(tenv, this, NULL));
                delete T;
                return NULL;
            }
            auto F = (LambdaType*) T;
            
            // Type the argument
            auto X = args[i]->typeOf(tenv);

            if (!X) {
                // The argument is untypable
                delete T;
                show_proof_therefore(type_res_str(tenv, this, NULL));
                return NULL;
            }

            show_proof_step("In order to permit the types, we require subsumption "
                    + X->toString() + " <: " + F->getLeft()->toString() + " to hold under " + tenv->toString());

            if (!X->equals(F->getLeft(), tenv)) {
                show_proof_step("The subsumption condition fails");
                show_proof_therefore(type_res_str(tenv, this, NULL));

                // Garbage collection
                delete F;
                delete X;
                return NULL;
            }

            X = new LambdaType("", X, tenv->make_tvar());

            // In the function tenv, we will unify the
            // argument types
            Type *Z;
            show_proof_step("To type " + toString()
                    + ", we must unify " + X->toString()
                    + " = " + F->toString() + " under "
                    + tenv->toString() + ".");
            
            // With the subsumption permitting, we attempt to unify the two sides.
            Z = X->unify(F, tenv);
            delete F;
            delete X;

            if (!Z) {
                // Non-unifiable
                show_proof_therefore(type_res_str(tenv, this, NULL));
                return NULL;
            }
                
            // Continue
            T = ((LambdaType*) Z)->getRight()->clone();
            delete Z;
        }
        
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

    T = type->clone();
    show_proof_therefore(type_res_str(tenv, this, T));
    return T;
}
Type* CompareExp::typeOf(Tenv tenv) {
    auto A = left->typeOf(tenv);
    auto B = right->typeOf(tenv);

    auto C = A->unify(B, tenv);
    delete A;
    delete B;

    if (C) {
        delete C;
        C = new BoolType;
    }

    show_proof_therefore(type_res_str(tenv, this, C));
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
/*
C |- x : s    C |- M : t
------------------------
   C |- d/dx M : dt/ds
*/
Type* DerivativeExp::typeOf(Tenv tenv) {
    auto Y = func->typeOf(tenv);

    if (Y) {
        Type *X;

        if (!isType<LambdaType>(Y)) {
            X = tenv->apply(var);
        } else {
            show_proof_step("We must extract type of arg " + var + " from " + Y->toString());
            Type *F = Y;
            while (typeid(*F) == typeid(LambdaType) && ((LambdaType*) F)->getX() != var)
                F = ((LambdaType*) F)->getRight();

            show_proof_step("Search reduces to subtype " + F->toString());

            if (typeid(*F) == typeid(LambdaType)) {
                X = ((LambdaType*) F)->getLeft()->clone();
                show_proof_step("Thus, yields " + var + " : " + X->toString());
            } else {
                show_proof_therefore(type_res_str(tenv, this, NULL));
                delete Y;
                return NULL;
            }
        }

        auto S = new DerivativeType(Y, X);
        
        show_proof_step("Thus, " + type_res_str(tenv, this, S) + " if it simplifies");

        auto T = S->subst(tenv);
        delete S;

        show_proof_therefore(type_res_str(tenv, this, T));
        return T;
    } else {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

}
Type* DictExp::typeOf(Tenv tenv) {
    auto trie = new Trie<Type*>;

    auto T = new DictType(trie);
    auto kt = keys->iterator();
    auto vt = vals->iterator();
    while (kt->hasNext()) {
        auto k = kt->next();
        auto v = vt->next();

        auto t = v->typeOf(tenv);
        if (!t) {
            show_proof_therefore(type_res_str(tenv, this, NULL));
            delete T;
            return NULL;
        } else
            trie->add(k, t);
    }

    show_proof_therefore(type_res_str(tenv, this, T));
    return T;
}
Type* DictAccessExp::typeOf(Tenv tenv) {
    Type *D = list->typeOf(tenv);

    Type *E = new DictType(new Trie<Type*>);

    Type *F = D->unify(E, tenv);
    delete D;
    delete E;

    if (!F) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    Type *T;
    if (((DictType*) F)->getTypes()->hasKey(idx))
        T = ((DictType*) F)->getTypes()->get(idx)->clone();
    else
        T = new VoidType;
    delete F;

    show_proof_therefore(type_res_str(tenv, this, T));

    return T;
}
Type* DivExp::typeOf(Tenv tenv) {
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
        
        int a = 0;
        int b = 0;

        while (isType<ListType>(A)) {
            a++;
            auto C = A;
            A = ((ListType*) C)->subtype()->clone();
            delete C;
        }
        while (isType<ListType>(B)) {
            b++;
            auto C = B;
            B = ((ListType*) C)->subtype()->clone();
            delete C;
        }

        if (a > 2 || b > 2) {
            // Do not go beyond 2nd order.
            delete A;
            delete B;
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        }

        auto C = A->unify(B, tenv);
        delete A;
        delete B;
        if (!C) {
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        } else if (!isType<RealType>(C) || !isType<IntType>(C)) {
            // The components must be numerical.
            delete C;
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        }

        switch (a+b) {
            case 4:
                C = new ListType(new ListType(C));
            case 3:
                C = new ListType(C);
            case 2:
                C = a == b ? C : new ListType(C);
            case 1:
                C = new ListType(C);
        }

        show_proof_therefore(type_res_str(tenv, this, C));
        return C;
    } else {
        C = new MultType(A, B);
        show_proof_therefore(type_res_str(tenv, this, C));
    }
    return C;
}
// Typing of f^g
Type* ExponentExp::typeOf(Tenv tenv) {
    Type *A = left->typeOf(tenv);
    if (!A) return NULL;

    Type *B = right->typeOf(tenv);
    if (!B) return NULL;
    
    /**
     * Now, we must impose two restrictions:
     * 1) log(f) must be typable
     * 2) e^(g log f) must be typable
     */
     
    Type *l = new SumType(new MultType(A->clone(), A->clone()), A);
    auto L = A->unify(l, tenv);
    delete l;

    if (!L) {
        delete B;
        show_proof_step("Base " + left->toString() + " cannot be typed properly in this context");
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    } else
        show_proof_step("Base types to " + L->toString());

    Type *r = new SumType(new MultType(B->clone(), B->clone()), B);
    auto R = B->unify(r, tenv);
    delete r;

    if (!R) {
        delete L;
        show_proof_step("Base " + right->toString() + " cannot be typed properly in this context");
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    } else
        show_proof_step("Exponent types to " + R->toString());
    
    MultType *M = new MultType(L, R);
    show_proof_step("Hence, we define " + L->toString() + " ^ " + R->toString() + " as " + M->toString());
    auto Y = M->subst(tenv);
    show_proof_step("Which simplifies to " + Y->toString());
    delete M;

    show_proof_therefore(type_res_str(tenv, this, Y));
    return Y;
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
    auto c = tenv->make_tvar();
    Type *G = new LambdaType("", b->clone(), new LambdaType("", a, c->clone()));
    Type *H = F->unify(G, tenv);
    
    delete F;
    delete G;

    if (H) {
        delete H;
        auto T = b->subst(tenv);
        b = T;
    } else {
        delete b;
        delete c;
        b = NULL;

        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    H = c->unify(b, tenv);

    delete b;
    delete c;

    show_proof_therefore(type_res_str(tenv, this, H));

    return H;
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
Type* ImportExp::typeOf(Tenv tenv) {

    // First, attempt to chack for a stdlib
    Type *T = type_stdlib(module);

    if (!T) {
        // Attempt to open the file
        ifstream file;
        file.open("./" + module + ".lom");
        
        // Ensure that the file exists
        if (!file) {
            throw_err("IO", "could not load module " + module);
            return NULL;
        }
        
        // Read the program from the input file
        int i = 0;
        string program = "";
        do {
            string s;
            getline(file, s);

            if (i++) program += " ";
            program += s;
        } while (file);

        throw_debug("module IO", "module " + module + " is defined by '" + program + "'\n");
        file.close();
        
        Exp modexp = compile(program);

        if (!modexp) {
            show_proof_therefore(type_res_str(tenv,this,NULL));
            return NULL;
        }

        T = modexp->typeOf(tenv);
        delete modexp;

        if (!T) {
            show_proof_therefore(type_res_str(tenv,this,NULL));
            return NULL;
        }
    }
    
    // Store the type
    tenv->set(name, T);
    
    // Evaluate the body
    Type *B = exp->typeOf(tenv);
    
    // Cleanup
    tenv->remove(name);
    
    // End of computation
    show_proof_therefore(type_res_str(tenv,this,B));
    return B;

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
        T = new LambdaType("", new VoidType, T);
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
            T = new LambdaType(xs[i], tenv->get_tvar(Ts[i]->toString())->clone(), T);
        }
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
Type* ListAddExp::typeOf(Tenv tenv) {
    auto L = list->typeOf(tenv);
    if (!L) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto E = elem->typeOf(tenv);
    if (!E) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        delete L;
        return NULL;
    }

    auto Ts = new ListType(E);

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
Type* ListRemExp::typeOf(Tenv tenv) {
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
    Type *t = new LambdaType("", a->clone(), tenv->make_tvar());
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
Type* MultExp::typeOf(Tenv tenv) {
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
        
        int a = 0;
        int b = 0;

        while (isType<ListType>(A)) {
            a++;
            auto C = A;
            A = ((ListType*) A)->subtype()->clone();
            delete C;
        }
        while (isType<ListType>(B)) {
            b++;
            auto C = B;
            B = ((ListType*) B)->subtype()->clone();
            delete C;
        }

        if (a > 2 || b > 2) {
            // Do not go beyond 2nd order.
            show_proof_step("multiplication is not defined beyond order 2");
            delete A;
            delete B;
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        } else {
            show_proof_step(
                    "We know that " + left->toString() + " is order " + to_string(a)
                    + " and " + right->toString() + " is order " + to_string(b) + ".");
        }

        auto C = A->unify(B, tenv);
        delete A;
        delete B;
        if (!C) {
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        } else if (!isType<RealType>(C) && !isType<IntType>(C)) {
            // The components must be numerical.
            delete C;
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        }

        show_proof_step("Hence, we can follow the order " + to_string(a+b) + " case with content type " + C->toString());

        switch (a+b) {
            case 4:
                C = new ListType(new ListType(C));
                break;
            case 3:
                C = new ListType(C);
                break;
            case 2:
                C = a == b ? C : new ListType(C);
                break;
            case 1:
                C = new ListType(C);
                break;
        }

        show_proof_therefore(type_res_str(tenv, this, C));
        return C;
    } else {
        C = new MultType(A, B);
        show_proof_therefore(type_res_str(tenv, this, C));
    }
    return C;
}
Type* NormExp::typeOf(Tenv tenv) {
    auto T = exp->typeOf(tenv);
    if (!T) {
        show_proof_therefore(type_res_str(tenv, this, T));
        return NULL;
    } else if (T->isConstant(tenv)) {
        // We can check numericality
        Type *R = T;
        int d = 0;
        while (isType<ListType>(R) && ++d <= 2)
            R = ((ListType*) R)->subtype();
        
        // Ban normalization of 3D or higher lists. We also verify sums.
        if (d > 2 || !isType<RealType>(R) && !isType<IntType>(R))
            R = NULL;
        else
            R = new RealType;
        
        delete T;

        show_proof_therefore(type_res_str(tenv, this, R));
        return NULL;
    } else {
        T = new MultType(T, tenv->make_tvar());
        auto U = T->subst(tenv);
        delete T;

        show_proof_therefore(type_res_str(tenv, this, U));
        return U;
    }
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
    if (!E) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        delete T;
        return NULL;
    }

    show_proof_step("Typing requires assignment: " + T->toString() + " = " + E->toString());

    Type *S = T->unify(E, tenv);

    delete E;
    delete T;

    if (S && typeid(*tgt) == typeid(VarExp)) {
        show_proof_step(tgt->toString() + " is a variable, hence we generalize its type to " + S->toString());
        tenv->set(tgt->toString(), S->clone());
    }
    
    show_proof_therefore(type_res_str(tenv, this, S));
    
    return S;
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
Type* StdMathExp::typeOf(Tenv tenv) {
    auto T = e->typeOf(tenv);
    auto R = new RealType;

    auto U = T->unify(R, tenv);
    delete T;
    delete R;

    return U;
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

    if (!C) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
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
