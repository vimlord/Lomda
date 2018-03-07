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

        // We now know that the types are both x -> y. Hence, we will change
        // the types.

        show_proof_therefore("under " + tenv->toString() + ", "
                + toString() + " = " + other->toString()
                + " unifies to (" + x->toString() + " -> " + y->toString() + ")");

        delete other->left;
        delete other->right;
        other->left = x->clone();
        other->right = y->clone();

        delete left;
        delete right;
        left = x->clone();
        right = y->clone();


        return new LambdaType(x, y);
    } else if (isType<VarType>(t)) {
        // Unify in the other direction.
        return t->unify(this, tenv);
    } else {
        show_mgu_step(tenv, this, t, NULL);
        return NULL;
    }
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
    } else if (isType<VarType>(t)) {
        // Unify in the other direction.
        return t->unify(this, tenv);
    } else {
        show_mgu_step(tenv, this, t, NULL);
        return NULL;
    }
}
Type* VarType::unify(Type* t, Tenv tenv) {
    // First, we simplify as far as we can go.
    // Be super careful that the variable isn't itself
    Type *A;
    if (tenv->get_tvar(name)->toString() != name) {
        A = tenv->get_tvar(name)->subst(tenv);
        tenv->set_tvar(name, A);
    } else
        A = clone();

    show_proof_step("We must unify type var " + name + " as " + A->toString() + " = " + t->toString() + ".");

    if (isType<VarType>(t)) {
        // Acquire the other variable
        VarType *v = (VarType*) t;

        // Simplify it
        Type *B = tenv->get_tvar(v->name)->subst(tenv);
        tenv->set_tvar(v->name, B);

        show_proof_step("Note that " + v->name + " = " + B->toString() + ".");

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

        if (!z) {
            // x and y have no unification. Hence, it is untypable.
            show_proof_therefore("under " + tenv->toString() + ", " + toString() + " = " + t->toString() + " is not unifiable");
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
            show_proof_therefore("under " + tenv->toString() + ", " + toString() + " = " + t->toString() + " is not unifiable");
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



