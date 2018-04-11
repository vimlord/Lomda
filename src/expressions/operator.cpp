#include "expressions/operator.hpp"

#include "value.hpp"
#include "config.hpp"
#include "types.hpp"

#include <string>
#include "math.hpp"

using namespace std;


OperatorExp::~OperatorExp() {
    delete left;
    delete right;
}

// Expression for adding stuff
Val AndExp::op(Value *a, Value *b) {
    
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
Val OrExp::op(Value *a, Value *b) {
    
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
Val DiffExp::op(Value *a, Value *b) {
    
    if (val_is_list(a)) {
        if (val_is_list(b)) {
            // Concatenate the two lists
            ArrayList<Val> *A = ((ListVal*) a)->get();
            ArrayList<Val> *B = ((ListVal*) b)->get();
            if (A->size() != B->size()) {
                Stringable *l = left ? (Stringable*) left : (Stringable*) a;
                Stringable *r = right ? (Stringable*) right : (Stringable*) b;
                throw_err("runtime", "cannot add lists " + l->toString() + " and " + r->toString() + " of differing lengths");
                return NULL;
            }

            ArrayList<Val> *C = new ArrayList<Val>;
            
            auto ait = A->iterator();
            auto bit = B->iterator();
            for (int i = 0; ait->hasNext(); i++) {
                Value *c = op(ait->next(), bit->next());
                if (!c) {
                    while (!C->isEmpty()) C->remove(0)->rem_ref();
                    return NULL;
                }
                C->add(i, c);
            }
            delete ait;
            delete bit;

            return new ListVal(C);
        } else  {
            Stringable *l = left ? (Stringable*) left : (Stringable*) a;
            Stringable *r = right ? (Stringable*) right : (Stringable*) b;
            throw_err("runtime", "addition is not defined between " + l->toString() + " and " + r->toString());
            return NULL;
        }
    } else if (val_is_number(a) && val_is_number(b)) {

        auto x = val_is_integer(a) ? ((IntVal*) a)->get() : ((RealVal*) a)->get();
        auto y = val_is_integer(b) ? ((IntVal*) b)->get() : ((RealVal*) b)->get();

        // Compute the result
        auto z = x - y;
        if (typeid(*a) == typeid(RealVal) || typeid(*b) == typeid(RealVal))
            return new RealVal(z);
        else
            return new IntVal(z);

    } else {
        Stringable *l = left ? (Stringable*) left : (Stringable*) a;
        Stringable *r = right ? (Stringable*) right : (Stringable*) b;
        throw_err("runtime", "subtraction is not defined between " + l->toString() + " and " + r->toString());
        return NULL;
    }

}

Val ExponentExp::op(Val a, Val b) {
    return pow(a, b);
}

// Perform Gaussian elimination on an n x n matrix
bool mtrx_inv(float** mtrx, int n) {
    // First, we aim to make the initial matrix upper triangular.
    // This is the only stage in which we will discover whether or
    // not our matrix is singular.
    for (int i = 0; i < n; i++) {
        int k = i;
        while (!mtrx[k][i] && ++k < n);
        
        if (k == n) return false;
        else if (k > i) {
            // Perform a row swap
            auto tmp = mtrx[i];
            mtrx[i] = mtrx[k];
            mtrx[k] = tmp;
        }

        for (int j = i+1; j < 2*n; j++)
            mtrx[i][j] /= mtrx[i][i];
        mtrx[i][i] = 1;

        for (int j = i+1; j < n; j++) {
            for (int k = i; k < 2*n; k++)
                mtrx[j][k] -= mtrx[i][k] * mtrx[j][i];
        }
    }

    // Now, we have assurances that there must exist an inverse.
    // Complete the inverse.
    for (int i = n-1; i >= 0; i--) {
        for (int j = 0; j < i; j++) {
            for (int k = n; k < 2*n; k++)
                mtrx[j][k] -= mtrx[i][k] * mtrx[j][i];
            
            mtrx[j][i] = 0;
        }
    }

    return true;

}

// Expression for multiplying studd
Val DivExp::op(Value *a, Value *b) {

    if (val_is_number(b)) {
        auto y = val_is_integer(b) ? ((IntVal*) b)->get() : ((RealVal*) b)->get();

        if (val_is_number(a)) {
            auto x = val_is_integer(a) ? ((IntVal*) a)->get() : ((RealVal*) a)->get();

            // Compute the result
            auto z = x / y;
            if (typeid(*a) == typeid(RealVal) || typeid(*b) == typeid(RealVal))
                return new RealVal(z);
            else
                return new IntVal(z);
        } else if (val_is_list(a)) {
            ListVal *c = new ListVal;
            auto it = ((ListVal*) a)->get()->iterator();

            while (c && it->hasNext()) {
                auto d = op(it->next(), b);
                if (!d) {
                    c->rem_ref();
                    c = NULL;
                } else
                    c->get()->add(c->get()->size(), d);
            }

            delete it;
            return c;
        } else {
            throw_err("runtime", "division is not defined between " + (left ? left->toString() : a->toString()) + " and " + (right ? right->toString() : b->toString()));
            return NULL;
        }
    } else {
        int *dta = typeid(*b) == typeid(ListVal) ? is_matrix(((ListVal*) b)) : NULL;
        if (!dta || dta[0] != dta[1]) {
            throw_err("runtime", "division is not defined between " + (left ? left->toString() : a->toString()) + " and " + (right ? right->toString() : b->toString()));
            return NULL;
        }

        // Thus, b is a matrix. We can now see if we can invert it via Gaussian reduction.
        int n = dta[0];
        delete dta;

        float **mtrx = new float*[n];
        for (int i = 0; i < n; i++) {
            mtrx[i] = new float[2*n];
            for (int j = 0; j < n; j++) {
                auto ij = (((ListVal*) ((ListVal*) b)->get()->get(i)))->get()->get(j);
                mtrx[i][j] = typeid(*ij) == typeid(IntVal) ? ((IntVal*) ij)->get() : ((RealVal*) ij)->get();
                mtrx[i][j+n] = i == j;
            }
        }

        bool nsing = mtrx_inv(mtrx, n);

        if (!nsing) {
            delete mtrx;
            throw_err("runtime", "matrix defined by " + (right ? right->toString() : b->toString()) + " is singular!");
            return NULL;
        }

        auto L = new ListVal;
        for (int i = 0; i < n; i++) {
            auto R = new ArrayList<Val>;
            L->get()->add(i, new ListVal(R));
            for (int j = 0; j < n; j++)
                R->add(j, new RealVal(mtrx[i][j+n]));
            delete[] mtrx[i];
        }
        delete[] mtrx;

        return L;
    }
}

Val CompareExp::op(Value *a, Value *b) {  

    if ((val_is_number(a) && val_is_number(b))) {
        auto A =
            typeid(*a) == typeid(IntVal)
            ? ((IntVal*) a)->get() :
            ((RealVal*) a)->get();

        auto B =
            typeid(*b) == typeid(IntVal)
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
    } else if (typeid(*a) == typeid(VoidVal) && typeid(*b) == typeid(VoidVal))
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
Val MultExp::op(Value *a, Value *b) {

    if (typeid(*a) == typeid(ListVal)) {
        if (typeid(*b) == typeid(ListVal)) {
            //std::cout << "compute " << *a << " * " << *b << "\n";

            if (((ListVal*) a)->get()->size() == 0) {
                throw_err("runtime", "multiplication is not defined on empty lists");
                return NULL;
            } else if (((ListVal*) b)->get()->size() == 0) {
                throw_err("runtime", "multiplication is not defined on empty lists");
                return NULL;
            }

            int ordA = 0, ordB = 0;
            for (Val A = a; typeid(*A) == typeid(ListVal); ordA++)
                A = ((ListVal*) A)->get()->get(0);

            for (Val B = b; typeid(*B) == typeid(ListVal); ordB++)
                B = ((ListVal*) B)->get()->get(0);
            
            // The restriction imposed is that multiplication restricts the
            // domain such that at least one of the arguments must be bounded
            // to the second order (matrices).
            if (ordA > 2 && ordB > 2) {
                throw_err("runtime", "multiplication is not defined on tensors of rank " + to_string(ordA) + " and " + to_string(ordB));
                return NULL;
            } else if (ordB > 2 && ordA > 2) {
                throw_err("runtime", "multiplication is not defined on tensors of rank " + to_string(ordA) + " and " + to_string(ordB));
                return NULL;
            }

            Val res = NULL;
            
            if (ordA > 1) {
                auto lst = new ArrayList<Val>;
                res = new ListVal(lst);

                MultExp mult(NULL, NULL);

                if (ordB == 1) {
                    //std::cout << "2 by 1\n";
                    // Matrix by vector
                    auto it = ((ListVal*) a)->get()->iterator();

                    while (res && it->hasNext()) {
                        Val v = it->next();
                        v = mult.op(v, b);
                        
                        if (!v) {
                            res->rem_ref();
                            res = NULL;
                        } else
                            lst->add(lst->size(), v);
                    }
                    delete it;
                } else {
                    // Matrix by matrix
                    //std::cout << "2 by 2\n";
                    
                    auto it = ((ListVal*) a)->get()->iterator();
                    while (res && it->hasNext()) {
                        Val v = mult.op(it->next(), b);
                        if (!v) {
                            res->rem_ref();
                            res = NULL;
                        } else
                            lst->add(lst->size(), v);
                    }

                    delete it;
                }

            } else if (ordB > 1) {
                //std::cout << "1 by 2\n";
                // List by matrix
                auto ait = ((ListVal*) a)->get()->iterator();
                auto bit = ((ListVal*) b)->get()->iterator();

                MultExp mult(NULL, NULL);
                SumExp sum(NULL, NULL);

                res = mult.op(ait->next(), bit->next());
                if (res)
                    //std::cout << "init: " << *res << "\n";
                
                while (res && ait->hasNext() && bit->hasNext()) {
                    Val u = mult.op(ait->next(), bit->next());
                    if (u) {
                        Val v = sum.op(res, u);
                        u->rem_ref();

                        res->rem_ref();
                        res = v;
                    } else {
                        res->rem_ref();
                        res = NULL;
                    }
                }

                if (ait->hasNext() || bit->hasNext()) {
                    if (res) res->rem_ref();
                    res = NULL;
                }

            } else {
                //std::cout << "1 by 1\n";
                // Dot product
                res = new IntVal;
                
                auto ait = ((ListVal*) a)->get()->iterator();
                auto bit = ((ListVal*) b)->get()->iterator();

                SumExp sum(NULL, NULL);
                
                while (res && ait->hasNext() && bit->hasNext()) {
                    Val A = ait->next();
                    Val B = bit->next();
                    Val v = op(A, B);
                    if (!v) {
                        res->rem_ref();
                        res = NULL;
                    } else {
                        Val s = sum.op(res, v);
                        v->rem_ref();

                        res->rem_ref();
                        res = s;
                    }
                }

                if (ait->hasNext() || bit->hasNext()) {
                    if (res) res->rem_ref();
                    res = NULL;
                }

                delete ait;
                delete bit;

            }

            if (!res) {
                if (left && right)
                    throw_err("runtime", "multiplication is not defined on non-matching lists (see: " + toString() + ")");
                else
                    throw_err("runtime", "multiplication is not defined on non-matching lists (see: " + (left ? left->toString() : a->toString()) + " * " + (right ? right->toString() : b->toString()) + ")");
            }

            return res;
        } else {
            ListVal *res = new ListVal;

            auto it = ((ListVal*) a)->get()->iterator();
            while (res && it->hasNext()) {
                Val x = op(it->next(), b);
                if (!x) {
                    res->rem_ref();
                    res = NULL;
                } else
                    res->get()->add(res->get()->size(), x);
            }

            delete it;
            return res;
        }
    } else if (val_is_list(b)) {
        return op(b, a);
    } else if (val_is_number(a) && val_is_number(b)) {

        // The lhs is numerical
        auto x = 
            typeid(*a) == typeid(IntVal)
            ? ((IntVal*) a)->get() :
              ((RealVal*) a)->get();
        
        if (typeid(*b) == typeid(ListVal)) {
            return op(b, a);
        } else {
            // rhs is numerical
            auto y = 
                typeid(*b) == typeid(IntVal)
                ? ((IntVal*) b)->get() :
                  ((RealVal*) b)->get();

            // Compute the result
            auto z = x * y;
            if (typeid(*a) == typeid(RealVal) || typeid(*b) == typeid(RealVal))
                return new RealVal(z);
            else
                return new IntVal(z);
        }
    } else {
        throw_err("runtime", "multiplication is not defined between " + (left ? left->toString() : a->toString()) + " and " + (right ? right->toString() : b->toString()));
        return NULL;
    }
}

// Expression for adding stuff
Val SumExp::op(Value *a, Value *b) {
    
    if (val_is_list(a)) {
        if (val_is_list(b)) {
            // Concatenate the two lists
            List<Val> *A = ((ListVal*) a)->get();
            List<Val> *B = ((ListVal*) b)->get();
            if (A->size() != B->size()) {
                Stringable *l = left ? (Stringable*) left : (Stringable*) a;
                Stringable *r = right ? (Stringable*) right : (Stringable*) b;
                throw_err("runtime", "cannot add lists " + l->toString() + " and " + r->toString() + " of differing lengths");
                return NULL;
            }

            auto C = new ArrayList<Val>;
            
            auto ait = A->iterator();
            auto bit = B->iterator();
            for (int i = 0; ait->hasNext(); i++) {
                Value *c = op(ait->next(), bit->next());
                if (!c) {
                    while (!C->isEmpty()) C->remove(0)->rem_ref();
                    return NULL;
                }
                C->add(i, c);
            }
            delete ait;
            delete bit;

            return new ListVal(C);
        } else  {
            Stringable *l = left ? (Stringable*) left : (Stringable*) a;
            Stringable *r = right ? (Stringable*) right : (Stringable*) b;
            throw_err("runtime", "addition is not defined between " + l->toString() + " and " + r->toString());
            return NULL;
        }
    } else if (val_is_number(a) && val_is_number(b)) {

        auto x = val_is_integer(a) ? ((IntVal*) a)->get() : ((RealVal*) a)->get();
        auto y = val_is_integer(b) ? ((IntVal*) b)->get() : ((RealVal*) b)->get();

        // Compute the result
        auto z = x + y;
        if (typeid(*a) == typeid(RealVal) || typeid(*b) == typeid(RealVal))
            return new RealVal(z);
        else
            return new IntVal(z);

    } else {
        Stringable *l = left ? (Stringable*) left : (Stringable*) a;
        Stringable *r = right ? (Stringable*) right : (Stringable*) b;
        throw_err("runtime", "addition is not defined between " + l->toString() + " and " + r->toString());
        return NULL;
    }

}
