#include "expressions/operator.hpp"

#include "value.hpp"
#include "config.hpp"
#include "types.hpp"

#include <string>
#include <cmath>

using namespace std;

int is_vector(ListVal *list) {
    auto it = list->get()->iterator();
    int i;
    for (i = 0; it->hasNext(); i++) {
        auto v = it->next();
        if (typeid(*v) != typeid(RealVal) && typeid(*v) != typeid(IntVal)) {
            delete it;
            return 0;
        }
    }

    delete it;
    return i;
}

int* is_matrix(ListVal *list) {
    auto it = list->get()->iterator();

    int cols = 0;

    // Empty lists are not matrices
    int i;
    for (i = 0; it->hasNext(); i++) {
        auto v = it->next();

        if (typeid(*v) == typeid(ListVal)) {
            int c = is_vector((ListVal*) v);
            if (i == 0) {
                if (c) {
                    cols = c;
                } else {
                    delete it;
                    return NULL;
                }
            } else if (!c || c != cols) {
                delete it;
                return NULL;
            }
        } else {
            delete it;
            return NULL;
        }
    }
    
    delete it;

    int *size = new int[2];
    size[0] = i;
    size[1] = cols;

    return size;
}

bool is_negligible_mtrx(ListVal *mtrx, double eps = 1e-4) {
    auto it = mtrx->get()->iterator();
    while (it->hasNext()) {
        ListVal *row = (ListVal*) it->next();
        auto jt = row->get()->iterator();
        while (jt->hasNext()) {
            Val v = jt->next();
            if (typeid(*v) == typeid(IntVal)) {
                return 0 == ((IntVal*) v)->get();
            } else {
                float f = ((RealVal*) v)->get();
                return f*f <= eps*eps;
            }
        }
        delete jt;
    }
    delete it;
    return true;
}

Val exp(Val v) {
    if (!v) return NULL;
    else if (typeid(*v) == typeid(RealVal))
        return new RealVal(exp(((RealVal*) v)->get()));
    else if (typeid(*v) == typeid(IntVal))
        return new RealVal(exp(((IntVal*) v)->get()));
    else if (typeid(*v) == typeid(ListVal)) {
        // Perhaps a matrix
        int *dim = is_matrix((ListVal*) v);
        if (!dim) {
            throw_err("runtime", "exponentiation is not defined on " + v->toString());
            return NULL;
        } else if (dim[0] != dim[1]) {
            delete[] dim;
            throw_err("runtime", "exponentiation is not defined on non-square matrix " + v->toString());
            return NULL;
        }

        // It is, so we can begin. Let there be a progress counter
        // that trachs X^n. We will initialize it for n = 0; X^n = I
        auto Xn = new ListVal;
        for (int i = 0; i < dim[0]; i++) {
            auto row = new ListVal;
            Xn->get()->add(i, row);
            for (int j = 0; j < dim[0]; j++)
                row->get()->add(j, new IntVal(i == j ? 1 : 0));
        }

        Val S = Xn->clone();
        MultExp mult(NULL, NULL);
        DivExp div(NULL, NULL);
        SumExp sum(NULL, NULL);
        
        // Now, we compute the Taylor polynomial
        for (int n = 1; !is_negligible_mtrx((ListVal*) Xn); n++) {
            // Raise Xn by a power
            auto Xn1 = mult.op(Xn,v);

            Xn->rem_ref();
            if (!Xn1) {
                S->rem_ref();
                return NULL;
            } else
                Xn = (ListVal*) Xn1;

            auto N = new IntVal(n);

            Xn1 = div.op(Xn,N);
            N->rem_ref();
            Xn->rem_ref();
            
            if (!Xn1) {
                S->rem_ref();
                return NULL;
            } else
                Xn = (ListVal*) Xn1;

            auto Su = sum.op(S, Xn);
            
            S->rem_ref();
            if (!Su) {
                Xn->rem_ref();
                return NULL;
            } else
                S = (ListVal*) Su;
        }

        return S;

    } else 
        return NULL;
}

Val log(Val v) {
    if (!v) return NULL;
    else if (typeid(*v) == typeid(RealVal))
        return new RealVal(log(((RealVal*) v)->get()));
    else if (typeid(*v) == typeid(IntVal))
        return new RealVal(log(((IntVal*) v)->get()));
    else {
        // Perhaps a matrix
        int *dim = is_matrix((ListVal*) v);
        if (!dim) {
            throw_err("runtime", "logarithm is not defined on " + v->toString());
            return NULL;
        } else if (dim[0] != dim[1]) {
            throw_err("runtime", "logarithm is not defined on non-square matrix " + v->toString());
            delete[] dim;
            return NULL;
        }
    
        // Initialize Xn = I
        auto I = new ListVal;
        for (int i = 0; i < dim[0]; i++) {
            auto row = new ListVal;
            I->get()->add(i, row);
            for (int j = 0; j < dim[0]; j++)
                row->get()->add(j, new IntVal(i == j ? 1 : 0));
        }

        MultExp mult(NULL, NULL);
        DivExp div(NULL, NULL);
        SumExp sum(NULL, NULL);
        DiffExp diff(NULL, NULL);
        
        // 1-x
        auto Xn = diff.op(v, I);
        auto X = diff.op(I, v);
        
        Val S = Xn->clone();

        auto term = Xn->clone();

        // Now, we compute the Taylor polynomial
        for (int n = 2; !is_negligible_mtrx((ListVal*) term); n++) {

            // Raise Xn by a power
            auto Xn1 = mult.op(Xn, X);

            Xn->rem_ref();
            if (!Xn1) {
                term->rem_ref();
                S->rem_ref();
                X->rem_ref();
                return NULL;
            } else
                Xn = Xn1;

            auto N = new RealVal(n);

            Xn1 = div.op(Xn, N);
            N->rem_ref();
            term->rem_ref();

            if (!Xn1) {
                S->rem_ref();
                v->rem_ref();
                return NULL;
            } else {
                term = Xn1;
            }
            
            auto Su = sum.op(S, Xn1);
            S->rem_ref();

            if (!Su) {
                term->rem_ref();
                Xn->rem_ref();
                X->rem_ref();
                return NULL;
            } else
                S = Su;
        }
        
        term->rem_ref();
        Xn->rem_ref();
        X->rem_ref();
        
        return S;
    }
}

Val identity_matrix(int n) {
    auto B = new ListVal;
    for (int i = 0; i < n; i++) {
        auto L = new ArrayList<Val>;
        B->get()->add(i, new ListVal(L));
        for (int j = 0; j < n; j++)
            L->add(j, new IntVal(i == j ? 1 : 0));
    }
    return B;
}

Val pow(Val b, Val p) {
    if (!b || !p) return NULL;
    else if (typeid(*p) == typeid(IntVal)) {
        MultExp mult(NULL, NULL);
        
        int n = ((IntVal*) p)->get();

        if (n == 0) {
            // Return the identity
        } else if (n > 0) {
            Val v;
            
            // We may need to generate an identity matrix
            if (typeid(*b) == typeid(ListVal)) {
                auto dta = is_matrix((ListVal*) b);
                if (dta && dta[0] == dta[1]) {
                    if (n == 1) {
                        // Anything to the 1st power is itself.
                        delete dta;
                        return b->clone();
                    }

                    v = identity_matrix(dta[0]);
                    delete dta;

                } else {
                    delete dta;
                    return NULL;
                }
            } else if (typeid(*b) == typeid(IntVal) || typeid(*b) == typeid(RealVal))
                v = new IntVal(1);
            
            // If n is nonzero, we need to prevent accidental GC
            if (n) b->add_ref();
            while (n) {
                Val u;
                if (n & 1) {
                    // Raise v up by the base
                    u = mult.op(v, b);
                    v->rem_ref();
                    v = u;
                }

                // Now, we progress the base
                u = mult.op(b, b);
                b->rem_ref();
                b = u;

                n >>= 1;
            }

            return v;

        } else {
            DivExp div(NULL, NULL);
            RealVal I(1);
            
            // Invert b
            b = div.op(&I, b);
            if (!b) {
                return NULL;
            }
            
            // Special handling for the minimum integer
            if (n-1 > n) {
                MultExp mult(NULL, NULL);
                auto B = mult.op(b, b);
                delete b;
                b = B;
                p = new IntVal(-(n/2));
            } else
                p = new IntVal(-n);
            
            // Compute the outcome
            Val v = pow(b, p);
            b->rem_ref();
            p->rem_ref();
            return v;
        }

    } else {
        MultExp mult(NULL, NULL);
        auto lnb = log(b);
        
        if (!lnb) {
            return NULL;
        }

        auto plnb = mult.op(p, lnb);

        lnb->rem_ref();

        if (!plnb)
            return NULL;
        
        auto y = exp(plnb);
        delete plnb;

        return y;
    }
}

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
        throw_err("runtime", "division is not defined between " + (left ? left->toString() : a->toString()) + " and " + (right ? right->toString() : b->toString()));
        return NULL;
    }
}

Val CompareExp::op(Value *a, Value *b) {  

    if (val_is_number(a) && val_is_number(b)) {
        auto A =
            typeid(*a) == typeid(BoolVal)
            ? ((BoolVal*) a)->get() :
            typeid(*a) == typeid(IntVal)
            ? ((IntVal*) a)->get() :
            ((RealVal*) a)->get();

        auto B =
            typeid(*b) == typeid(BoolVal)
            ? ((BoolVal*) b)->get() :
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
                return NULL;
        }

    } else if (typeid(*a) == typeid(VoidVal) && typeid(*b) == typeid(VoidVal))
        return new BoolVal(operation == CompOp::EQ);
    else
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

            if (ordA > 2) {
                throw_err("runtime", "multiplication is not defined on tensors of rank " + to_string(ordA));
                return NULL;
            }

            for (Val B = b; typeid(*B) == typeid(ListVal); ordB++)
                B = ((ListVal*) B)->get()->get(0);

            if (ordB > 2) {
                throw_err("runtime", "multiplication is not defined on tensors of rank " + to_string(ordB));
                return NULL;
            }

            Val res = NULL;
            
            if (ordA == 2) {
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

            } else if (ordB == 2) {
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
