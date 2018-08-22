#include "math.hpp"
#include "expression.hpp"

#include <cmath>
#include <string>

using namespace std;

int is_vector(Val v) {
    if (!isVal<ListVal>(v))
        return 0;
    ListVal *list = (ListVal*) v;

    auto it = list->iterator();
    int i;
    for (i = 0; it->hasNext(); i++) {
        auto v = it->next();
        if (!isVal<RealVal>(v) && !isVal<IntVal>(v)) {
            delete it;
            return 0;
        }
    }

    delete it;
    return i;
}

int* is_matrix(Val v) {
    if (!isVal<ListVal>(v))
        return 0;
    ListVal *list = (ListVal*) v;

    int cols = 0;

    // Empty lists are not matrices
    int i;
    for (i = 0; i < list->size(); i++) {
        auto v = list->get(i);

        if (isVal<ListVal>(v)) {
            int c = is_vector((ListVal*) v);
            if (i == 0) {
                if (c) {
                    cols = c;
                } else {
                    return NULL;
                }
            } else if (!c || c != cols) {
                return NULL;
            }
        } else {
            return NULL;
        }
    }

    int *size = new int[2];
    size[0] = i;
    size[1] = cols;

    return size;
}

int is_square_matrix(Val v) {
    if (!isVal<ListVal>(v))
        return 0;
    ListVal *list = (ListVal*) v;

    // Empty lists are not matrices
    int i;
    for (i = 0; i < list->size(); i++) {
        Val row =list->get(i);

        if (isVal<ListVal>(row)) {
            int c = is_vector((ListVal*) row);
            if (!c || c != list->size()) {
                return 0;
            }
        } else {
            return 0;
        }
    }

    return list->size();
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

// For building representations of vectors and matrices
float* extract_vector(ListVal *list) {
    float *vec = new float[list->size()];
    for (int i = 0; i < list->size(); i++) {
        Val v = list->get(i);
        vec[i] = isVal<RealVal>(v) ? ((RealVal*) v)->get() : ((IntVal*) v)->get();
    }
    return vec;
}
float** extract_matrix(ListVal *list) {
    float **vec = new float*[list->size()];
    for (int i = 0; i < list->size(); i++) {
        vec[i] = extract_vector((ListVal*) list->get(i));
    }
    return vec;
}

bool is_negligible_mtrx(ListVal *mtrx, double eps = 1e-4) {
    auto it = mtrx->iterator();
    while (it->hasNext()) {
        ListVal *row = (ListVal*) it->next();
        auto jt = row->iterator();
        while (jt->hasNext()) {
            Val v = jt->next();
            if (isVal<IntVal>(v)) {
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

Val dot(Val A, Val B) {
    if ((isVal<IntVal>(A) || isVal<RealVal>(A)) || (isVal<IntVal>(B) || isVal<RealVal>(B))) {
        return mult(A, B);
    } else if (isVal<ListVal>(A) || isVal<ListVal>(B)) {
        ListVal *lA = (ListVal*) A;
        ListVal *lB = (ListVal*) B;
        if (lA->size() != lB->size()) {
            throw_err("runtime", "cannot compute arbitrary dot product of " + A->toString() + " and " + B->toString());
            return NULL;
        }

        Val v = NULL;
        for (int i = 0; i < lA->size(); i++) {
            Val d = dot(lA->get(i), lB->get(i));
            if (!d) {
                throw_err("runtime", "cannot compute arbitrary dot product of " + A->toString() + " and " + B->toString());
                v->rem_ref();
            }

            if (v) {
                Val s = add(v, d);
                d->rem_ref();
                d = s;
                v->rem_ref();
            }

            v = d;
        }

        if (!v) {
            throw_err("runtime", "cannot compute arbitrary dot product of " + A->toString() + " and " + B->toString());
        }

        return v;
    } else {
        throw_err("runtime", "cannot compute arbitrary dot product of " + A->toString() + " and " + B->toString());
        return NULL;
    }
}

Val exp(Val v) {
    if (!v) return NULL;
    else if (isVal<RealVal>(v))
        return new RealVal(exp(((RealVal*) v)->get()));
    else if (isVal<IntVal>(v))
        return new RealVal(exp(((IntVal*) v)->get()));
    else if (isVal<ListVal>(v)) {
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
            Xn->add(i, row);
            for (int j = 0; j < dim[0]; j++)
                row->add(j, new IntVal(i == j ? 1 : 0));
        }

        Val S = Xn->clone();
        
        
        
        
        // Now, we compute the Taylor polynomial
        for (int n = 1; !is_negligible_mtrx((ListVal*) Xn); n++) {
            // Raise Xn by a power
            auto Xn1 = mult(Xn,v);

            Xn->rem_ref();
            if (!Xn1) {
                S->rem_ref();
                return NULL;
            } else
                Xn = (ListVal*) Xn1;

            auto N = new IntVal(n);

            Xn1 = div(Xn,N);
            N->rem_ref();
            Xn->rem_ref();
            
            if (!Xn1) {
                S->rem_ref();
                return NULL;
            } else
                Xn = (ListVal*) Xn1;

            auto Su = add(S, Xn);
            
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
    else if (isVal<RealVal>(v))
        return new RealVal(log(((RealVal*) v)->get()));
    else if (isVal<IntVal>(v))
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
        // We will also compute the norm
        float norm = 0;
        for (int i = 0; i < dim[0]; i++) {
            auto row = new ListVal;
            I->add(i, row);
            for (int j = 0; j < dim[0]; j++) {
                // Update the norm
                Val x = ((ListVal*) ((ListVal*) v)->get(i))->get(j);
                float f = isVal<RealVal>(x) ? ((RealVal*) x)->get() : ((IntVal*) x)->get();
                norm += f*f;

                row->add(j, new IntVal(i == j ? 1 : 0));
            }
        }

        
        

        if (norm >= 1) {
            // If the norm is too large, we must scale back our matrix to stay
            // within the bounds of natural log taylor poly (|x| < 1)
            //std::cout << "sqnorm is " << norm << "\n";

            float a = norm;

            //std::cout << "let a = " << a << "\n";

            auto A = new ListVal;
            for (int i = 0; i < dim[0]; i++) {
                auto row = new ListVal;
                A->add(i, row);
                for (int j = 0; j < dim[0]; j++) {
                    // Update the norm
                    Val x = ((ListVal*) ((ListVal*) v)->get(i))->get(j);
                    float f = isVal<RealVal>(x) ? ((RealVal*) x)->get() : ((IntVal*) x)->get();
                    row->add(j, new RealVal(f / a));
                }
            }

            auto logA = log(A);
            A->rem_ref();

            if (!logA) {
                delete[] dim;
                I->rem_ref();
                return NULL;
            }

            //std::cout << "log(A/a) = " << *logA << "\n";

            // Scale I
            float loga = log(a);
            //std::cout << "log a = " << loga << "\n";
            for (int i = 0; i < dim[0]; i++) {
                ((ListVal*) ((ListVal*) I)->get(i))->get(i)->rem_ref();
                ((ListVal*) ((ListVal*) I)->get(i))->set(i, new RealVal(loga));
            }

            delete[] dim;

            //std::cout << "log(aI) = I log a = " << *I << "\n";

            auto Y = add(I, logA);

            I->rem_ref();
            logA->rem_ref();
             
            return Y;
        }

        delete[] dim;
        
        // 1-x
        Val Xn = sub(v, I);
        auto X = sub(I, v);
        
        Val S = Xn->clone();

        auto term = Xn->clone();

        // Now, we compute the Taylor polynomial
        for (int n = 2; !is_negligible_mtrx((ListVal*) term); n++) {

            // Raise Xn by a power
            auto Xn1 = mult(Xn, X);

            Xn->rem_ref();
            if (!Xn1) {
                term->rem_ref();
                S->rem_ref();
                X->rem_ref();
                return NULL;
            } else
                Xn = Xn1;

            Val N = new RealVal(n);

            Xn1 = div(Xn, N);
            N->rem_ref();
            term->rem_ref();

            if (!Xn1) {
                S->rem_ref();
                v->rem_ref();
                return NULL;
            } else {
                term = Xn1;
            }
            
            auto Su = add(S, Xn1);
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
        auto L = new ListVal;
        B->add(i, L);
        for (int j = 0; j < n; j++)
            L->add(j, new IntVal(i == j ? 1 : 0));
    }
    return B;
}

Val pow(Val b, Val p) {
    if (!b || !p) return NULL;
    else if (isVal<IntVal>(p)) {
        
        
        int n = ((IntVal*) p)->get();

        if (n == 0) {
            // Return the identity
            if (isVal<RealVal>(b) || isVal<IntVal>(b))
                return new IntVal(1);

            auto dim = is_matrix(b);
            if (!dim) {
                throw_err("runtime", "logarithm is not defined on " + b->toString());
                return NULL;
            } else if (dim[0] != dim[1]) {
                throw_err("runtime", "logarithm is not defined on non-square matrix " + b->toString());
                delete[] dim;
                return NULL;
            }

            auto I = identity_matrix(dim[0]);
            delete[] dim;

            return I;

        } else if (n == 1) {
            // Identity
            return b->clone();
        } else if (n > 0) {
            Val v = NULL;
            
            // We may need to generate an identity matrix
            if (isVal<ListVal>(b)) {
                auto dta = is_matrix((ListVal*) b);
                if (dta && dta[0] == dta[1]) {
                    v = identity_matrix(dta[0]);
                    delete dta;

                } else {
                    delete dta;
                    return NULL;
                }
            } else if (isVal<IntVal>(b) || isVal<RealVal>(b))
                v = new IntVal(1);
            
            // If n is nonzero, we need to prevent accidental GC
            if (n) b->add_ref();
            while (n) {
                Val u;
                if (n & 1) {
                    // Raise v up by the base
                    u = mult(v, b);
                    v->rem_ref();
                    v = u;
                }

                // Now, we progress the base
                u = mult(b, b);
                b->rem_ref();
                b = u;

                n >>= 1;
            }

            b->rem_ref();

            return v;

        } else {
            
            RealVal I(1);
            
            // Invert b
            b = inv(b);
            if (!b) {
                return NULL;
            }
            
            // Special handling for the minimum integer
            if (n-1 > n) {
                
                auto B = mult(b, b);
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
        
        auto lnb = log(b);
        
        if (!lnb) {
            return NULL;
        }

        auto plnb = mult(p, lnb);

        lnb->rem_ref();

        if (!plnb)
            return NULL;
        
        auto y = exp(plnb);
        delete plnb;

        return y;
    }
}

Val add(Val a, Val b) {
    if (val_is_list(a)) {
        if (val_is_list(b)) {
            // We will do an element-wise addition
            List<Val> *A = ((ListVal*) a);
            List<Val> *B = ((ListVal*) b);
            if (A->size() != B->size()) {
                throw_err("runtime", "cannot add lists " + a->toString() + " and " + b->toString() + " of differing lengths");
                return NULL;
            }

            auto C = new ListVal;
            
            auto ait = A->iterator();
            auto bit = B->iterator();
            for (int i = 0; ait->hasNext(); i++) {
                Val c = add(ait->next(), bit->next());
                if (!c) {
                    while (!C->isEmpty()) C->remove(0)->rem_ref();
                    return NULL;
                }
                C->add(i, c);
            }
            delete ait;
            delete bit;
            
            return C;

        } else  {
            throw_err("runtime", "addition is not defined between " + a->toString() + " and " + b->toString());
            return NULL;
        }
    } else if (val_is_number(a) && val_is_number(b)) {

        auto x = val_is_integer(a) ? ((IntVal*) a)->get() : ((RealVal*) a)->get();
        auto y = val_is_integer(b) ? ((IntVal*) b)->get() : ((RealVal*) b)->get();

        // Compute the result
        auto z = x + y;
        if (isVal<RealVal>(a) || isVal<RealVal>(b))
            return new RealVal(z);
        else
            return new IntVal(z);

    } else {
        throw_err("runtime", "addition is not defined between " + a->toString() + " and " + b->toString());
        return NULL;
    }
}

Val sub(Val a, Val b) {
    if (val_is_list(a)) {
        if (val_is_list(b)) {
            // We will do an element-wise subtraction
            List<Val> *A = ((ListVal*) a);
            List<Val> *B = ((ListVal*) b);
            if (A->size() != B->size()) {
                throw_err("runtime", "cannot subtract lists " + a->toString() + " and " + b->toString() + " of differing lengths");
                return NULL;
            }

            auto C = new ListVal;
            
            auto ait = A->iterator();
            auto bit = B->iterator();
            for (int i = 0; ait->hasNext(); i++) {
                Val c = sub(ait->next(), bit->next());
                if (!c) {
                    while (!C->isEmpty()) C->remove(0)->rem_ref();
                    return NULL;
                }
                C->add(i, c);
            }
            delete ait;
            delete bit;
            
            return C;

        } else  {
            throw_err("runtime", "subtraction is not defined between " + a->toString() + " and " + b->toString());
            return NULL;
        }
    } else if (val_is_number(a) && val_is_number(b)) {

        auto x = val_is_integer(a) ? ((IntVal*) a)->get() : ((RealVal*) a)->get();
        auto y = val_is_integer(b) ? ((IntVal*) b)->get() : ((RealVal*) b)->get();

        // Compute the result
        auto z = x - y;
        if (isVal<RealVal>(a) || isVal<RealVal>(b))
            return new RealVal(z);
        else
            return new IntVal(z);

    } else {
        throw_err("runtime", "subtraction is not defined between " + a->toString() + " and " + b->toString());
        return NULL;
    }
}

Val mult(Val a, Val b) {
    if (isVal<ListVal>(a)) {
        if (isVal<ListVal>(b)) {
            //std::cout << "compute " << *a << " * " << *b << "\n";

            if (((ListVal*) a)->size() == 0) {
                throw_err("runtime", "multiplication is not defined on empty lists");
                return NULL;
            } else if (((ListVal*) b)->size() == 0) {
                throw_err("runtime", "multiplication is not defined on empty lists");
                return NULL;
            }

            int ordA = 0, ordB = 0;
            for (Val A = a; isVal<ListVal>(A); ordA++)
                A = ((ListVal*) A)->get(0);

            for (Val B = b; isVal<ListVal>(B); ordB++)
                B = ((ListVal*) B)->get(0);
            
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
                auto lst = new ListVal;
                res = lst;

                if (ordB == 1) {
                    //std::cout << "2 by 1\n";
                    // Matrix by vector
                    auto lstA = (ListVal*) a;
                    
                    for (int i = 0; res && i < lstA->size(); i++) {
                        Val v = lstA->get(i);
                        v = mult(v, b);
                        
                        if (!v) {
                            res->rem_ref();
                            res = NULL;
                        } else
                            lst->add(lst->size(), v);
                    }
                } else {
                    // Matrix by matrix
                    //std::cout << "2 by 2\n";
                    auto lstA = (ListVal*) a;

                    for (int i = 0; res && i < lstA->size(); i++) {
                        Val v = mult(lstA->get(i), b);
                        if (!v) {
                            res->rem_ref();
                            return NULL;
                        } else
                            lst->add(lst->size(), v);
                    }
                }

            } else if (ordB > 1) {
                //std::cout << "1 by 2\n";
                // List by matrix
                auto lstA = (ListVal*) a;
                auto lstB = (ListVal*) b;

                if (lstA->size() != lstB->size()) {
                    throw_err("runtime", "multiplication is not defined on non-matching lists (see: " + a->toString() + " * " + b->toString() + ")");
                    return NULL;
                }

                res = mult(lstA->get(0), lstB->get(0));
                
                for (int i = 1; res && i < lstA->size(); i++) {
                    Val u = mult(lstA->get(i), lstB->get(i));
                    if (u) {
                        Val v = add(res, u);
                        u->rem_ref();

                        res->rem_ref();
                        res = v;
                    } else {
                        res->rem_ref();
                        return NULL;
                    }
                }

            } else {
                // Dot product
                auto lstA = (ListVal*) a;
                auto lstB = (ListVal*) b;

                if (lstA->size() != lstB->size()) {
                    throw_err("runtime", "multiplication is not defined on non-matching lists (see: " + a->toString() + " * " + b->toString() + ")");
                    return NULL;
                }

                res = new IntVal;
                
                for (int i = 0; res && i < lstA->size(); i++) {
                    Val A = lstA->get(i);
                    Val B = lstB->get(i);
                    Val v = mult(A, B);
                    if (!v) {
                        res->rem_ref();
                        return NULL;
                    } else {
                        Val s = add(res, v);
                        v->rem_ref();

                        res->rem_ref();
                        res = s;
                    }
                }
            }

            return res;
        } else {
            ListVal *res = new ListVal;

            auto it = ((ListVal*) a)->iterator();
            while (res && it->hasNext()) {
                Val x = mult(it->next(), b);
                if (!x) {
                    res->rem_ref();
                    res = NULL;
                } else
                    res->add(res->size(), x);
            }

            delete it;
            return res;
        }
    } else if (val_is_list(b)) {
        return mult(b, a);
    } else if (isVal<AdtVal>(a)) {
        AdtVal *A = (AdtVal*) a;
        
        // Get the arg count
        int argc;
        for (argc = 0; A->getArgs()[argc]; argc++);
        
        // Generate an argument list
        Val *args = new Val[argc+1];

        for (int i = 0; i < argc; i++) {
            // Compute the element-wise product
            args[i] = mult(A->getArgs()[i], b);
            if (!args[i]) {
                // The product is non-computable
                while (i--) args[i]->rem_ref();
                delete[] args;
                return NULL;
            }
        }
        args[argc] = NULL;
        
        return new AdtVal(A->getType(), A->getKind(), args);


    } else if (isVal<AdtVal>(b)) {
        return mult(b, a);
    } else if (val_is_number(a) && val_is_number(b)) {

        // The lhs is numerical
        auto x = 
            isVal<IntVal>(a)
            ? ((IntVal*) a)->get() :
              ((RealVal*) a)->get();
        
        if (isVal<ListVal>(b)) {
            return mult(b, a);
        } else {
            // rhs is numerical
            auto y = 
                isVal<IntVal>(b)
                ? ((IntVal*) b)->get() :
                  ((RealVal*) b)->get();

            // Compute the result
            auto z = x * y;
            if (isVal<RealVal>(a) || isVal<RealVal>(b))
                return new RealVal(z);
            else
                return new IntVal(z);
        }
    } else {
        throw_err("runtime", "multiplication is not defined between " + a->toString() + " and " + b->toString());
        return NULL;
    }
}

Val inv(Val b) {
    
    // Primitive cases
    if (isVal<IntVal>(b)) return new IntVal(1 / ((IntVal*) b)->get());
    if (isVal<RealVal>(b)) return new RealVal(1 / ((RealVal*) b)->get());

    int *dta = isVal<ListVal>(b) ? is_matrix(((ListVal*) b)) : NULL;
    if (!dta || dta[0] != dta[1]) {
        throw_err("runtime", "value " + b->toString() + " is not an square matrix");
        return NULL;
    }

    // Thus, b is a matrix. We can now see if we can invert it via Gaussian reduction.
    int n = dta[0];
    delete dta;

    float **mtrx = new float*[n];
    for (int i = 0; i < n; i++) {
        mtrx[i] = new float[2*n];
        for (int j = 0; j < n; j++) {
            auto ij = (((ListVal*) ((ListVal*) b)->get(i)))->get(j);
            mtrx[i][j] = isVal<IntVal>(ij) ? ((IntVal*) ij)->get() : ((RealVal*) ij)->get();
            mtrx[i][j+n] = i == j;
        }
    }

    bool nsing = mtrx_inv(mtrx, n);

    if (!nsing) {
        delete mtrx;
        throw_err("runtime", "matrix defined by " +  b->toString() + " is singular!");
        return NULL;
    }

    auto L = new ListVal;
    for (int i = 0; i < n; i++) {
        auto R = new ListVal;
        L->add(i, R);
        for (int j = 0; j < n; j++)
            R->add(j, new RealVal(mtrx[i][j+n]));
        delete[] mtrx[i];
    }
    delete[] mtrx;

    return L;
}

Val div(Val a, Val b) {

    if (val_is_number(b)) {
        auto y = val_is_integer(b) ? ((IntVal*) b)->get() : ((RealVal*) b)->get();

        if (val_is_number(a)) {
            auto x = val_is_integer(a) ? ((IntVal*) a)->get() : ((RealVal*) a)->get();

            // Compute the result
            auto z = x / y;
            if (isVal<RealVal>(a) || isVal<RealVal>(b))
                return new RealVal(z);
            else
                return new IntVal(z);
        } else if (val_is_list(a)) {
            ListVal *c = new ListVal;
            auto it = ((ListVal*) a)->iterator();

            while (c && it->hasNext()) {
                auto d = div(it->next(), b);
                if (!d) {
                    c->rem_ref();
                    c = NULL;
                } else
                    c->add(c->size(), d);
            }

            delete it;
            return c;
        } else {
            throw_err("runtime", "division is not defined between " +  a->toString() + " and " + b->toString());
            return NULL;
        }
    } else {
        // Replace b with its inverse, reducing to multiplication.
        b = inv(b);
        
        // Multiply the two values.
        Val c = mult(a, b);
        
        // Garbage colletion
        b->rem_ref();
        
        return c;

    }
}

