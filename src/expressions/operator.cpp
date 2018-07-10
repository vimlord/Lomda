#include "expressions/operator.hpp"

#include "value.hpp"
#include "config.hpp"
#include "types.hpp"

#include <string>
#include <cmath>

#include "math.hpp"

using namespace std;


OperatorExp::~OperatorExp() {
    delete left;
    delete right;
}

// Expression for adding stuff
Val AndExp::op(Val a, Val b) {
    
    if (!val_is_bool(a) || !val_is_bool(b)) {
        throw_err("runtime", "boolean operations are not defined on non-booleans (see: '" + toString() + "')");
        return NULL;
    }

    auto x = ((BoolVal*) a)->get();
    auto y = ((BoolVal*) b)->get();
    
    // Compute the result
    auto z = x && y;
    return new BoolVal(z);

}

// Expression for adding stuff
Val OrExp::op(Val a, Val b) {
    
    if (!val_is_bool(a) || !val_is_bool(b)) {
        throw_err("runtime", "boolean operations are not defined on non-booleans (see: '" + toString() + "')");
        return NULL;
    }

    auto x = ((BoolVal*) a)->get();
    auto y = ((BoolVal*) b)->get();
    
    // Compute the result
    auto z = x || y;
    return new BoolVal(z);

}

// Expression for multiplying studd
Val DiffExp::op(Val a, Val b) {
    return sub(a, b);
}

Val ExponentExp::op(Val a, Val b) {
    return pow(a, b);
}

// Expression for multiplying studd
Val DivExp::op(Val a, Val b) {
    return div(a, b);
}

Val CompareExp::op(Val a, Val b) {  

    if ((val_is_number(a) && val_is_number(b))) {
        auto A =
            isVal<IntVal>(a)
            ? ((IntVal*) a)->get() :
            ((RealVal*) a)->get();

        auto B =
            isVal<IntVal>(b)
            ? ((IntVal*) b)->get() :
            ((RealVal*) b)->get();

        switch (operation) {
            case EQ:
                return new BoolVal(A == B);
            case NEQ:
                return new BoolVal(A != B);
            case GT:
                return new BoolVal(A > B);
            case LT:
                return new BoolVal(A < B);
            case GEQ:
                return new BoolVal(A >= B);
            case LEQ:
                return new BoolVal(A <= B);
            default:
                return new BoolVal(false);
        }
    } else if (val_is_bool(a) && val_is_bool(b)) {
        auto A = ((BoolVal*) a)->get();
        auto B = ((BoolVal*) b)->get();

        switch (operation) {
            case EQ:
                return new BoolVal(A == B);
            case NEQ:
                return new BoolVal(A != B);
            default:
                return new BoolVal(false);
        }
    } else if (isVal<VoidVal>(a) && isVal<VoidVal>(b))
        return new BoolVal(operation == CompOp::EQ);
    else if (val_is_string(a) && val_is_string(b)) {
        string A = a->toString();
        string B = b->toString();

        switch (operation) {
            case EQ:
                return new BoolVal(A == B);
            case NEQ:
                return new BoolVal(A != B);
            case GT:
                return new BoolVal(A > B);
            case LT:
                return new BoolVal(A < B);
            case GEQ:
                return new BoolVal(A >= B);
            case LEQ:
                return new BoolVal(A <= B);
            default:
                return new BoolVal(false);
        }
    } else
        return new BoolVal(false);
}

// Expression for multiplying studd
Val MultExp::op(Val a, Val b) {
    return mult(a, b);
}

// Expression for adding stuff
Val SumExp::op(Val a, Val b) {
    return add(a, b);
}

Val ModulusExp::op(Val a, Val b) {
    if (val_is_integer(a) && val_is_integer(b)) {
        auto x = ((IntVal*) a)->get();
        auto y = ((IntVal*) b)->get();

        return new IntVal(x % y);

    } else if (val_is_number(a) && val_is_number(b)) {

        auto x = val_is_integer(a) ? ((IntVal*) a)->get() : ((RealVal*) a)->get();
        auto y = val_is_integer(b) ? ((IntVal*) b)->get() : ((RealVal*) b)->get();

        // Compute the result
        auto z = fmod(x,y);
        if (isVal<RealVal>(a) || isVal<RealVal>(b))
            return new RealVal(z);
        else
            return new IntVal(z);

    } else {
        Stringable *l = left ? (Stringable*) left : (Stringable*) a;
        Stringable *r = right ? (Stringable*) right : (Stringable*) b;
        throw_err("runtime", "modulus is not defined between " + l->toString() + " and " + r->toString());
        return NULL;
    }
}

