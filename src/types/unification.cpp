#include "types.hpp"
#include "expression.hpp"
#include "proof.hpp"

using namespace std;

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
Type* DictType::unify(Type *t, Tenv tenv) {
    if (isType<PrimitiveType>(t)) {
        return NULL;
    } else if (isType<DictType>(t)) {
        // The result dictionary type
        DictType *D = new DictType(new Trie<Type*>);

        DictType *B = (DictType*) t;
        
        // Handle all of the type records of this side of the unity
        auto it = types->iterator();
        while (it->hasNext()) {
            auto k = it->next();
            auto t = types->get(k);

            Type *T;

            if (B->types->hasKey(k)) {
                auto p = B->types->get(k);
                
                // The unification of t = p
                T = t->unify(p, tenv);

                if (!T) {
                    show_proof_therefore("under "+tenv->toString()+", "+toString()+" = "+t->toString()+" is not unifiable");
                    delete D;
                    delete it;
                    return NULL;
                }

            } else
                // Add the type
                T = t->clone();

            D->types->add(k, T);
        }
        delete it;

        // Handle the rest of the unity
        it = B->types->iterator();
        while (it->hasNext()) {
            auto k = it->next();

            if (!D->types->hasKey(k))
                D->types->add(k, B->types->get(k));

        }
        delete it;

        return D;

    } else if (isType<VarType>(t))
        return t->unify(this, tenv);
    else
        return NULL;

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
    if (isType<PrimitiveType>(t)) {
        return NULL;
    } else if (isType<LambdaType>(t)) {
        LambdaType *other = (LambdaType*) t;
        
        show_proof_step("First, we unify the left; suppose " + left->toString() + " = " + other->left->toString() + ".");
        auto x = left->unify(other->left, tenv);
        if (!x) {
            show_proof_step("Thus, the lhs does not unify.");
            show_proof_therefore("under " + tenv->toString() + ", "
                + toString() + " = " + other->toString()
                + " is not unifiable");
            return NULL;
        }

        show_proof_step("Next, we unify the right; suppose " + right->toString() + " = " + other->right->toString() + ".");
        auto y = right->unify(other->right, tenv);
        if (!y) {
            show_proof_step("Thus, the rhs does not unify.");
            show_proof_therefore("under " + tenv->toString() + ", "
                + toString() + " = " + other->toString()
                + " is not unifiable");
            delete x;
            return NULL;
        }

        // We now know that the types are both x -> y. Hence, we will change
        // the types.
        
        // Immediately simplify wherever possible.
        delete other->left;
        delete other->right;
        other->left = x->clone();
        other->right = y->clone();

        delete left;
        delete right;
        left = x->clone();
        right = y->clone();

        Type *T;
        
        // We require that the identifiers match.
        if (id == other->id)
            T = new LambdaType(id, x, y);
        else if (other->id == "" || id == "")
            T = new LambdaType(id + other->id, x, y);
        else
            T = NULL;
        
        show_proof_therefore("under " + tenv->toString() + ", "
                + toString() + " = " + other->toString()
                + " unifies to (" + x->toString() + " -> " + y->toString() + ")");

        return T;

    } else if (isType<VarType>(t)) {
        // Unify in the other direction.
        return t->unify(this, tenv);
    } else {
        show_mgu_step(tenv, this, t, NULL);
        return NULL;
    }
}
Type* ListType::unify(Type* t, Tenv tenv) {
    if (isType<PrimitiveType>(t)) {
        return NULL;
    } else if (isType<ListType>(t)) {
        auto A = type;
        auto B = ((ListType*) t)->type;
        auto C = A->unify(B, tenv);

        if (C) {
            C = new ListType(C);
            show_proof_therefore("under " + tenv->toString() + ", " + toString() + " = " + t->toString() + " unifies to " + C->toString());
        } else {
            show_proof_therefore("under " + tenv->toString() + ", " + toString() + " = " + t->toString() + " is not unifiable");
            return NULL;
        }

        return C;
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
    } else if (isType<VarType>(t)) {
        // Unify in the other direction.
        return t->unify(this, tenv);
    } else {
        show_mgu_step(tenv, this, t, NULL);
        return NULL;
    }
}
Type* VarType::unify(Type* t, Tenv tenv) {
    if (t->depends_on_tvar(name, tenv)) {
        show_proof_step("Type " + t->toString() + " is already defined by " + name);
        show_proof_therefore("unification implies that " + name + " is a recursive type");
        show_proof_therefore("under " + tenv->toString() + ", " + name + " = " + t->toString() + " is not unifiable");
        return NULL;
    }

    // First, we simplify as far as we can go.
    // Be super careful that the variable isn't itself
    Type *A;
    if (tenv->get_tvar(name)->toString() != name) {
        A = tenv->get_tvar(name)->subst(tenv);
        tenv->set_tvar(name, A);
        A = A->clone();
    } else
        A = clone();

    show_proof_step("We must unify type var " + name + " as " + A->toString() + " = " + t->toString() + ".");
    
    if (isType<VarType>(t)) {
        // Acquire the other variable
        VarType *v = (VarType*) t;

        // Simplify it
        Type *B = tenv->get_tvar(v->name)->subst(tenv);
        tenv->set_tvar(v->name, B);

        // Trivial equivalence test
        if (name == v->name) {
            show_proof_step(name + " = " + v->name + " trivially.");
            if (A->toString() == B->toString())
                A = A->clone();
            else if (!isType<VarType>(A))
                A = A->clone();
            else
                A = B->clone();
            
            tenv->set_tvar(name, A->clone());

            show_proof_therefore("under " + tenv->toString() + ", " + name + " = " + t->toString() + " unifies to " + A->toString());

            return A;
        }

        show_proof_step("We know that " + v->name + " = " + B->toString());

        Type* x;
        if (isType<VarType>(A) && isType<VarType>(B) && A->toString() == B->toString()) {
            // They are equivalent
            show_proof_step(A->toString() + " and " + B->toString() + " are trivially equivalent");
            x = new VarType(A->toString());
        } else if (A->toString() != name || B->toString() != t->toString())
            // Unify the two types
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
        
        // Simplify the other side
        auto S = t->subst(tenv);

        show_proof_step("The right hand simplifies to " + S->toString() + ".");

        Type *T;
        // We must verify that this variable checks out
        if (A->toString() != name) {
            // The variable is definitely not itself
            T = A->unify(S, tenv);
            delete S;
        } else
            // Since the variable identifies as itself, we can
            // define it to be whatever we want it to be.
            T = S;
        
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
    auto T = t->subst(tenv);

    if (isType<SumType>(T)) {
        show_proof_step("We seek to unify " + toString() + " = " + T->toString() + " by unifying the two halves.");
        SumType* other = (SumType*) T;

        auto x = left->unify(other->left, tenv);
        if (!x) {
            delete T;
            return NULL;
        }

        delete left;
        delete other->left;
        left = x;
        other->left = x->clone();

        auto y = right->unify(other->right, tenv);
        if (!y) {
            delete T;
            return NULL;
        }

        delete other->right;
        delete right;
        right = y;
        other->right = y->clone();
        
        // Next, we can try unifying x and y
        auto z = x->unify(y, tenv);

        if (!z) {
            // x and y have no unification. Hence, it is untypable.
            show_proof_therefore("under " + tenv->toString() + ", " + toString() + " = " + t->toString() + " is not unifiable");
            delete T;
            return NULL;
        } else {
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
            delete T;
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

            delete T;
            
            return new SumType(z, z->clone());
        } else
            show_proof_therefore("under " + tenv->toString() + ", " + toString() + " = " + t->toString() + " is not unifiable");
            // There does not exist a unification
            delete T;
            return NULL;
    } else if (isType<RealType>(T)) {
        show_proof_step("We seek to unify " + toString() + " = " + T->toString() + " to the fundamental form.");

        auto x = left->unify(T, tenv);
        if (!x) {
            delete T;
            return NULL;
        }

        delete left;
        left = x;
        
        x = right->unify(T, tenv);
        delete T;
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

    } else if (isType<VarType>(T) || isType<ListType>(T)) {
        show_proof_step("We seek to unify " + toString() + " = " + T->toString() + " to a simpler form.");
        
        Type *U = subst(tenv);
        show_proof_step("The left side simplifies to " + U->toString());

        Type *X = T->unify(U, tenv);

        if (X) {
            show_proof_step("Hence, " + T->toString() + " = " + U->toString() + " reduces to " + X->toString());
            show_proof_therefore("under " + tenv->toString() + ", " + T->toString() + ", " + toString() + " / " + X->toString());
        }

        delete T;
        delete U;
        
        return X;
    }


    show_proof_step("We are unable to unify " + toString() + " = " + T->toString() + " under " + tenv->toString() + ".");
    delete T;
    return NULL;
}

Type* MultType::unify(Type* t, Tenv tenv) {
    auto T = t->subst(tenv);

    if (isType<MultType>(T)) {
        show_proof_step("We seek to unify " + toString() + " = " + T->toString() + " by unifying the two halves.");
        auto M = (MultType*) T;
        
        // Unify the left
        auto x = left->unify(M->left, tenv);
        if (!x) {
            delete T;
            return NULL;
        }

        delete left;
        delete M->left;
        left = x;
        M->left = x->clone();
        
        // Unify the right
        auto y = right->unify(M->right, tenv);
        if (!y) {
            delete T;
            return NULL;
        }

        delete M->right;
        delete right;
        right = y;
        M->right = y->clone();

        auto z = subst(tenv);
        delete T;

        return z;

    } else if (isType<VarType>(T)) {
        auto U = subst(tenv);
        auto V = T->unify(U,tenv);

        delete T;
        delete U;

        return V;
    } else if (isType<RealType>(T)) {
        show_proof_step("We seek to unify " + toString() + " = " + T->toString() + " to the fundamental form.");
        
        auto U = subst(tenv);

        show_proof_step("The left hand side simplifies to " + U->toString());
        show_proof_step("Now, we unify " + T->toString() + " = " + U->toString());

        auto V = T->unify(U, tenv);

        if (!V)
            show_proof_step("We are unable to unify " + toString() + " = " + T->toString() + " under " + tenv->toString() + ".");
        else
            show_proof_step("Hence, " + T->toString() + " = " + U->toString() + " reduces to " + V->toString());

        delete T;
        delete U;

        return V;
    }

    show_proof_step("We are unable to unify " + toString() + " = " + T->toString() + " under " + tenv->toString() + ".");
    delete T;
    return NULL;
}

Type* DerivativeType::unify(Type* t, Tenv tenv) {
    auto T = t->subst(tenv);
    
    if (isType<RealType>(right) || isType<IntType>(left)) {
        // Base case: derivative of type wrt number is the type
        auto U = T->unify(left, tenv);
        delete T;
        return U;
    } else if (isType<DerivativeType>(t)) {
        show_proof_step("We seek to unify " + toString() + " = " + T->toString() + " by unifying the two halves.");
        auto D = (DerivativeType*) T;

        // Unify the left
        auto x = left->unify(D->left, tenv);
        if (!x) {
            delete T;
            return NULL;
        }

        delete left;
        delete D->left;
        left = x;
        D->left = x->clone();
        
        // Unify the right
        auto y = right->unify(D->right, tenv);
        if (!y) {
            delete T;
            return NULL;
        }

        delete D->right;
        delete right;
        right = y;
        D->right = y->clone();

        auto z = subst(tenv);
        delete T;

        return z;

    } else if (isType<VarType>(T)) {
        auto U = subst(tenv);
        auto V = T->unify(U,tenv);

        delete T;
        delete U;
        
        return V;
    } else if (isType<IntType>(T) || isType<RealType>(T)) {
        // If we can reduce the typing, then we can conclude the
        // top half of the expression.


        // Output type is same as other side.
        auto Y = left->unify(T, tenv);
        
        if (!Y) {
            delete T;
            show_proof_step("We are unable to unify " + toString() + " = " + T->toString() + " under " + tenv->toString() + ".");
            return NULL;
        }

        delete left;
        left = Y;

        // We know that the bottom must be a real number
        auto R = new RealType;
        auto X = right->unify(R, tenv);
        delete R;

        if (!X) {
            delete T;
            show_proof_step("We are unable to unify " + toString() + " = " + T->toString() + " under " + tenv->toString() + ".");
            return NULL;
        }

        delete right;
        right = X;
        
        // The result is the type f the top.
        return T;
    } else if (isType<ListType>(T)) {
        auto L = (ListType*) T;
        auto E = L->subtype();

        if (isType<ListType>(left)) {
            auto lY = (ListType*) left;
            auto Y = lY->subtype();
            
            // Subderivative
            auto dYdX = new DerivativeType(Y->clone(), right->clone());
            
            // Unify the two types
            Y = dYdX->unify(E, tenv);
            delete dYdX;
            
            // Thus, if Y is defined, we have an answer.
            if (Y) Y = new ListType(Y);
            else show_proof_step("We are unable to unify " + toString() + " = " + T->toString() + " under " + tenv->toString() + ".");

            return Y;
        } else if (isType<RealType>(left) || isType<IntType>(left)) {
            if (isType<ListType>(right)) {
                auto lX = (ListType*) right;
                auto X = lX->subtype();

                // Subderivative
                auto dYdX = new DerivativeType(left->clone(), X->clone());

                auto Y = dYdX->unify(E, tenv);
                delete dYdX;

                // Thus, if Y is defined, we have an answer.
                if (Y) Y = new ListType(Y);
                else show_proof_step("We are unable to unify " + toString() + " = " + T->toString() + " under " + tenv->toString() + ".");

                return Y;
            }
        }
    }

    delete T;
    show_proof_step("We are unable to unify " + toString() + " = " + T->toString() + " under " + tenv->toString() + ".");
    return NULL;
}



