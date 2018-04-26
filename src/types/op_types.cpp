#include "types.hpp"
#include "expression.hpp"
#include "proof.hpp"

bool MultType::equals(Type *t, Tenv tenv) {
    return left->equals(t, tenv) || right->equals(t, tenv);
}
bool SumType::equals(Type *t, Tenv tenv) {
    return left->equals(t, tenv) || right->equals(t, tenv);
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
    auto C = T->subst(tenv);
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
    Type *C = T->subst(tenv);
    delete T;

    show_proof_therefore(type_res_str(tenv, this, C));
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
    Type *C = T->subst(tenv);
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
    auto C = T->subst(tenv);
    delete T;
    show_proof_therefore(type_res_str(tenv, this, C));
    return C;
}

