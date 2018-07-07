#include "types.hpp"
#include "expression.hpp"
#include "proof.hpp"

Type* MultType::simplify(Tenv tenv) {
    auto L = left->simplify(tenv);
    auto R = right->simplify(tenv);
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

Type* SumType::simplify(Tenv tenv) {
    // Reduce the left
    auto L = left->simplify(tenv);
    if (!L) return NULL;
    
    // Reduce the right
    auto R = right->simplify(tenv);
    if (!R) { delete L; return NULL; }
    
    // We can make certain reductions where necessary
    if (isType<ListType>(L) || isType<ListType>(R)) {
        // We will reduce this type to a list if possible.
        auto l = new ListType(tenv->make_tvar());
        auto T = l->unify(isType<ListType>(L) ? L : R, tenv);
        delete l;
        
        // The cast could not be completed
        if (!T) return NULL;
        else delete T;
        
        // Simplify both sides again (they will both be lists)
        auto L = (ListType*) left->simplify(tenv);
        auto R = (ListType*) right->simplify(tenv);
        delete left; delete right;
        left = L; right = R;
        
        // Now, we reduce the newly formed list type
        l = new ListType(new SumType(L->subtype()->clone(), R->subtype()->clone()));
        T = l->simplify(tenv);
        delete l;

        return T;
    } else if (L->isConstant(tenv) && R->isConstant(tenv)) {
        // In this scenario, given that neither side is a list, both
        // sides MUST be a number.
        if (isType<RealType>(L) && isType<RealType>(R))
            return L->unify(R, tenv);
        else
            return NULL;
    } else
        return clone();
}

Type* DerivativeType::simplify(Tenv tenv) {
    // Reduce the left
    auto L = left->simplify(tenv);
    if (!L) return NULL;
    
    // Reduce the right
    auto R = right->simplify(tenv);
    if (!R) { delete L; return NULL; }

    delete left; delete right;
    left = L; right = R;

    if (isType<AlgebraicDataType>(L)) {
        // We require that the derivative be of the same type.
        R = unify(L, tenv);
        delete R;
        
        return R ?  L->clone() : NULL;
        
    } else if (isType<ListType>(L)) {
        // d/dt [s] = [ds/dt]
        R = new DerivativeType(((ListType*) L)->subtype()->clone(), R->clone());
        L = R->simplify(tenv);
        delete R;

        if (L) return new ListType(L);
        else return NULL;
        
    } else if (isType<RealType>(L) || isType<IntType>(L)) {
        // Base case: dZ/dZ = dZ/dR = Z, dR/dZ = dR/dR = R
        if (isType<RealType>(R) || isType<IntType>(R))
            return L->clone();
        else if (isType<ListType>(R)) {
            R = new DerivativeType(L->clone(), ((ListType*) R)->subtype()->clone());
            L = R->simplify(tenv);
            delete R;

            if (L) return new ListType(L);
            else return NULL;
        } else if (isType<TupleType>(R)) {
            R = new TupleType(
                new DerivativeType(L->clone(), ((TupleType*) R)->getLeft()->clone()),
                new DerivativeType(L->clone(), ((TupleType*) R)->getRight()->clone()));

            L = R->simplify(tenv);
            delete R;
            
            return L;
        } else if (isType<VarType>(R))
            return clone();
        else
            return NULL;

        return R->clone();
    } else if (isType<TupleType>(L)) {
        // d/dx (a * b) = (da/dx * db/dx)
        R = new TupleType(
            new DerivativeType(((TupleType*) L)->getLeft()->clone(), R->clone()),
            new DerivativeType(((TupleType*) L)->getRight()->clone(), R->clone()));

        L = R->simplify(tenv);
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
        auto Y = R->simplify(tenv);
        delete R;
         
        // Check the validity of the substitution generated.
        if (!Y) return NULL;
        
        // d(x->y)/dz = x->(dy/dz)
        return new LambdaType(F->getX(), X, Y);
    } else if (isType<SumType>(L)) {
        // d/dx a + b = da/dx + db/dx

        SumType *S = (SumType*) L;
        auto T = new SumType(
                new DerivativeType(S->getLeft()->clone(), R->clone()),
                new DerivativeType(S->getRight()->clone(), R->clone())
        );
        auto U = T->simplify(tenv);
        delete T;
        
        return U;
    } else if (isType<MultType>(L)) {
        // d/ds a*b = a*db/ds + b*da/ds

        MultType *S = (MultType*) L;
         
        // Build both sides
        auto dL= new DerivativeType(S->getLeft()->clone(), R->clone());
        auto dR = new DerivativeType(S->getRight()->clone(), R->clone());
        
        auto T = new SumType(
            new MultType(L->clone(), dR),
            new MultType(R->clone(), dL)
        );

        auto U = T->simplify(tenv);
        delete T;
        
        return U;
    } else if (isType<DictType>(L)) {
        auto D = (DictType*) L;

        auto dD = new DictType;
        
        // d/dx {x1 : t1, ...} = {x1 : dt1/dx, ...}
        auto it = D->getTypes()->iterator();
        while (it->hasNext()) {
            auto x = it->next();
            auto t = D->getTypes()->get(x);
            
            // Build a most general type for the argument
            auto dt = new DerivativeType(t->clone(), R->clone());
            if (!dt) {
                delete dD;
                return NULL;
            }
            
            // Reduce our guess
            auto T = dt->simplify(tenv);
            delete dt;
            if (!T) {
                delete dD;
                return NULL;
            }
            
            // Add the discovered subtype to our end result
            dD->getTypes()->add(x, dt);
        }

        return dD;

    } else
        return NULL;
}

// Typing rules that evaluate to type U - V
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
    auto T = new SumType(A, B);
    auto C = T->simplify(tenv);
    delete T;
    show_proof_therefore(type_res_str(tenv, this, C));
    return C;
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
    Type *T = new MultType(A, B);
    Type *C = T->simplify(tenv);
    delete T;

    show_proof_therefore(type_res_str(tenv, this, C));
    return C;
}

// Typing of f^g
Type* ExponentExp::typeOf(Tenv tenv) {
    Type *A = left->typeOf(tenv);
    if (!A) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    Type *B = right->typeOf(tenv);
    if (!B) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        delete A;
        return NULL;
    }
    
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
    auto Y = M->simplify(tenv);
    show_proof_step("Which simplifies to " + Y->toString());
    delete M;

    show_proof_therefore(type_res_str(tenv, this, Y));
    return Y;
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
    Type *T = new MultType(A, B);
    Type *C = T->simplify(tenv);
    delete T;

    show_proof_therefore(type_res_str(tenv, this, C));
    return C;
}

Type* ModulusExp::typeOf(Tenv tenv) {
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

    // Modulus is essentially a subcomponent of division. Hence, we treat it
    // as such when evaluating the type.
    Type *T = new MultType(A, B);
    Type *C = T->simplify(tenv);
    delete T;

    show_proof_therefore(type_res_str(tenv, this, C));
    return C;
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
    auto T = new SumType(A, B);
    show_proof_step("We suppose that " + type_res_str(tenv, this, T));
    show_proof_step("If we can reduce " + T->toString() + ", then this will hold.");
    auto C = T->simplify(tenv);
    delete T;
    show_proof_therefore(type_res_str(tenv, this, C));
    return C;
}

