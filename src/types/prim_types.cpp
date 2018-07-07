#include "types.hpp"
#include "expression.hpp"
#include "proof.hpp"


// Typing of primitives
Type* VarExp::typeOf(Tenv tenv) {
    auto T = tenv->apply(id);
    auto U = T->simplify(tenv); // Attempt a reduction of the variable type

    delete T;
    T = U;

    if (T) // Perform an update.
        tenv->set(id, T->clone());

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

