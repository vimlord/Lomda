#include "expression.hpp"

using namespace std;

Exp Expression::symb_diff(string x) {
    return new DerivativeExp(clone(), x);
}

Exp SumExp::symb_diff(string x) {
    auto L = left->symb_diff(x);
    if (!L) return NULL;
    auto R = right->symb_diff(x);
    if (!R) { delete L; return NULL; }

    return new SumExp(L, R);
}

Exp DiffExp::symb_diff(string x) {
    auto L = left->symb_diff(x);
    if (!L) return NULL;
    auto R = right->symb_diff(x);
    if (!R) { delete L; return NULL; }

    return new DiffExp(L, R);
}

Exp ModulusExp::symb_diff(string x) {
    auto L = left->symb_diff(x);
    if (!L) return NULL;
    auto R = right->symb_diff(x);
    if (!R) { delete L; return NULL; }

    // d/dx A mod B = A' - B' floor(A/B)
    return new DiffExp(
        L,
        new MultExp(
            R,
            new CastExp(
                new IntType,
                new DivExp(
                    left->clone(),
                    right->clone()
                )
            )
        )
    );
}

Exp MultExp::symb_diff(string x) {
    auto L = left->symb_diff(x);
    if (!L) return NULL;
    auto R = right->symb_diff(x);
    if (!R) { delete L; return NULL; }

    return new SumExp(new MultExp(left->clone(), R), new MultExp(right->clone(), L));
}

Exp DivExp::symb_diff(string x) {
    auto L = left->symb_diff(x);
    if (!L) return NULL;
    auto R = right->symb_diff(x);
    if (!R) { delete L; return NULL; }

    return new DivExp(new DiffExp(new MultExp(right->clone(), L), new MultExp(left->clone(), R)), new MultExp(right->clone(), right->clone()));
}

Exp StdMathExp::symb_diff(string x) {
    auto dx = e->symb_diff(x);
    if (!dx) return NULL;

    switch (fn) {
        case SIN:
            return new MultExp(new StdMathExp(COS, e->clone()), dx);
        case COS:
            return new MultExp(new IntExp(-1), new StdMathExp(SIN, e->clone()));
        case TAN:
            return new DivExp(dx,
                    new MultExp(
                        new StdMathExp(COS, e->clone()),
                        new StdMathExp(COS, e->clone())
                    ));
        case ASIN:
            return new DivExp(dx,
                new StdMathExp(SQRT, new DiffExp(
                    new IntExp(1),
                    new ExponentExp(e->clone(), new IntExp(2))
                )));
        case ACOS:
            return new DivExp(new MultExp(new IntExp(-1), dx),
                new StdMathExp(SQRT, new DiffExp(
                    new IntExp(1),
                    new ExponentExp(e->clone(), new IntExp(2))
                )));
        case ATAN:
            return new DivExp(dx,
                new SumExp(
                    new IntExp(1),
                    new ExponentExp(e->clone(), new IntExp(2))
            ));
        case LOG:
            return new DivExp(dx, e->clone());
        case SQRT:
            return new DivExp(dx, new MultExp(new IntExp(2), new StdMathExp(SQRT, e->clone())));
        case EXP:
            return new MultExp(clone(), dx);
        default:
            delete dx;
            return new DerivativeExp(clone(), x);
    }

    delete dx;
    return NULL;
}

Exp ThunkExp::symb_diff(string x) {
    auto E = exp->symb_diff(x);
    if (!E) return NULL;
    else return new ThunkExp(E);
}

Exp DerivativeExp::symb_diff(string x) {
    // Derive the sublayer.
    auto dy = func->symb_diff(var);
    if (!dy) return NULL;
    // Derive this layer.
    auto dx = dy->symb_diff(x);

    // GC
    delete dy;
    
    // d^2f/dxy
    return dx;
}


