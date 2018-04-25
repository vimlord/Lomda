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

    Exp du = NULL;

    switch (fn) {
        case SIN:
            return new MultExp(new StdMathExp(COS, e->clone()), dx);
        case COS:
            return new MultExp(new IntExp(-1), new StdMathExp(SIN, e->clone()));
        case LOG:
            return new DivExp(dx, e->clone());
        case SQRT:
            return new DivExp(dx, new MultExp(new IntExp(2), new StdMathExp(SQRT, e->clone())));
        case EXP:
            return new MultExp(e->clone(), dx);
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
    auto dx = dy->symb_diff(var);

    // GC
    delete dy;
    
    // d^2f/dxy
    return dx;
}


