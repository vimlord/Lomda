#include "math.hpp"
#include "expression.hpp"
#include <cmath>

int is_vector(Val v) {
    if (typeid(*v) != typeid(ListVal))
        return 0;
    ListVal *list = (ListVal*) v;

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

int* is_matrix(Val v) {
    if (typeid(*v) != typeid(ListVal))
        return 0;
    ListVal *list = (ListVal*) v;

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

