#include "stdlib.hpp"

#include "expressions/stdlib.hpp"
#include "expression.hpp"

#include "math.hpp"
#include <cmath>

/**
 * Divide a polynomial by a first order polynomial; P(x) / (x - c)
 */
inline float* synthetic_division(float *P, float c, int n) {
    float *Q = new float[n];

    float d = 0;

    for (int i = n; i > 0; i--) {
        d = P[i] + d*c;
        Q[i-1] = d;
    }

    return Q;
}

inline float* derivative(float *F, int n) {
    float *G = new float[n];
    for (int i = 0; i < n; i++)
        G[i] = F[i+1] / (i+1);
    return G;
}

inline float apply(float *F, int n, float x) {
    float y = 0;
    for (int i = n; i >= 0; i--)
        y = x*y + F[i];

    return y;
}

float* factorize(float *F, int n) {
    float *xs = new float[n];

    float R = 0;
    for (int i = 0; i < n; i++)
        R += F[i] > 0 ? F[i] : -F[i];
    if (R < 1) R = 1;

    for (int i = 0; n > 0; i++) {
        // Perform a Newton-Raphson estimation. We wil use a random
        // initial point to estimate.
        float x = fmod(rand(), 2*R) - R;
        float y;
        float *dF = derivative(F, n);

        bool complete = false;
        while (true) {
            // Compute y
            y = apply(F, n, x);
            if (y == 0) {
                complete = true;
                break;
            } else if (std::isinf(y))
                break;

            float dy = apply(dF, n-1, x);
            if (dy == 0) {
                break;
            }

            x -= y/dy;
        }

        delete[] dF;

        if (!complete) {
            // Start over
            i--;
            continue;
        }

        // Store the answer.
        xs[i] = x;

        // Modify the polynomial.
        if (i) {
            delete[] F;

            if (i < n-1) {
                float *G = synthetic_division(F, x, n);
                F = G;
            }
        } else
            F = synthetic_division(F, x, n);

        n--;
    }

    return xs;
}

float* characteristic_poly(ListVal *A) {
    int n = A->size();

    // We will need to generate a characteristic polynomial.
    // Start with the coefficients.
    float *c = new float[n+1];
    c[n] = 1;

    auto AM = new ListVal;
    for (int i = 0; i < n; i++) {
        auto row = new ListVal;
        AM->add(i, row);
        for (int j = 0; j < n; j++)
            row->add(j, new IntVal);
    }

    for (int k = 1; k <= n; k++) {
        // First, we update M

        auto I = new ListVal;
        for (int i = 0; i < n; i++) {
            auto row = new ListVal;
            I->add(i, row);
            for (int j = 0; j < n; j++)
                row->add(j, i == j ? new RealVal(c[n-k+1]) : new RealVal);
        }

        auto M = (ListVal*) add(AM, I);
        AM->rem_ref();
        I->rem_ref();

        // Then, we update AM
        AM = (ListVal*) mult(A, M);
        M->rem_ref();

        // Then, we will calculate the trace.
        float tr = 0;
        for (int i = 0; i < n; i++)
            tr += ((RealVal*) ((ListVal*) AM->get(i))->get(i))->get();

        // And from it, we compute the coefficient.
        c[n-k] = -tr / k;
    }

    AM->rem_ref();

    return c;
}

Val std_transpose(Env env) {
    Val x = env->apply("x");

    if (!isVal<ListVal>(x)) {
        throw_err("type", "linalg.transpose : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
        return NULL;
    }

    ListVal *xss = (ListVal*) x;

    int rows = 0, cols = 0;
    
    auto it = xss->iterator();
    while (it->hasNext()) {
        Val xs = it->next();
        if (!isVal<ListVal>(xs)) {
            throw_err("type", "linalg.transpose : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
            delete it;
            return NULL;
        }

        int c = 0;
        auto jt = ((ListVal*) xs)->iterator();
        while (jt->hasNext()) {
            auto v = jt->next();
            if (!isVal<RealVal>(v) && !isVal<IntVal>(v)) {
                throw_err("type", "linalg.transpose : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
                delete it;
                delete jt;
                return NULL;
            }
            c++;
        }
        delete jt;

        if (rows && c != cols) {
            throw_err("type", "linalg.transpose : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
            delete it;
            return NULL;
        } else
            cols = c;

        rows++;
    }
    delete it;
    
    ListVal *yss = new ListVal;
    for (int i = 0; i < cols; i++) {
        // Init the row structure
        ListVal *ys = new ListVal;
        yss->add(i, ys);

        // Add the row transpose
        for (int j = 0; j < rows; j++) {
            ListVal *row = (ListVal*) (xss->get(j));
            Val y = row->get(i);
            ys->add(j, y);
            y->add_ref();
        }
    }

    return yss;
}

/**
 * Computes a QR decomposition using the Gram-Schmidt process.
 */
Val std_qr(Env env) {
    Val x = env->apply("x");

    if (!isVal<ListVal>(x)) {
        throw_err("type", "linalg.qr : [[R]] -> [[[R]]] cannot be applied to argument "
                + x->toString());
    }
    
    // We want the column matrix form
    auto A = (ListVal*) std_transpose(env);
    if (!A) return NULL;
    
    auto Q = new ListVal;
    auto R = new ListVal;

    auto it = A->iterator();
    
    if (!it->hasNext()) {
        throw_err("type", "linalg.qr : [[R]] -> [[[R]]] cannot be applied to argument "
                + env->apply("x")->toString());
        delete it;
        Q->rem_ref();
        return NULL;
    }

    while (it->hasNext()) {
        x = it->next();
        if (!isVal<ListVal>(x)) {
            throw_err("type", "linalg.qr : [[R]] -> [[[R]]] cannot be applied to argument "
                    + env->apply("x")->toString());
            delete it;
            Q->rem_ref();
            return NULL;
        }
        
        // The current col of A
        auto a = (ListVal*) x;
        
        // Initial condition for col of Q
        auto u = a->clone();

        auto jt = Q->iterator();
        while (jt->hasNext()) {
            auto e = jt->next();

            // proj a onto v = a*e * e
            auto n = mult(a, e);
            e = mult(n, e);
            n->rem_ref();
            
            // Subtract the projection
            n = sub(u, e);
            u->rem_ref();
            e->rem_ref();

            u = (ListVal*) n;
        }

        // Compute the sqnorm of our vector u
        float norm = 0;
        jt = u->iterator();
        while (jt->hasNext()) {
            auto y = jt->next();

            if (isVal<IntVal>(y))
                norm += pow(((IntVal*) y)->get(), 2);
            else if (isVal<RealVal>(y))
                norm += pow(((RealVal*) y)->get(), 2);
            else {
                throw_err("type", "linalg.qr : [[R]] -> [[[R]]] cannot be applied to argument "
                        + env->apply("x")->toString());
                delete jt;
                delete it;
                Q->rem_ref();
                R->rem_ref();
                return NULL;
            }
        }
        
        // Now, we know the norm of u
        norm = sqrt(norm);
        
        // Update Q
        RealVal N(norm);
        auto e = div(u, &N);
        u->rem_ref();

        Q->add(Q->size(), e);

        // Now, we compute a row of R
        auto r = new ListVal;
        R->add(R->size(), r);
        jt = A->iterator();
        
        bool compute = false;
        while (jt->hasNext()) {
            auto y = jt->next();

            if (!compute)
                compute = y == x;

            if (compute) {
                // Apply the dot product.
                y = mult(y, e);
                if (!y) {
                    delete it;
                    delete jt;
                    Q->rem_ref();
                    R->rem_ref();
                    return NULL;
                }
                r->add(r->size(), y);
            } else {
                // By definition, we know it to be 0
                r->add(r->size(), new RealVal(0));
            }
        }
    }
    
    // Our result for Q is its transpose. We will correct this.
    auto Qt = new ListVal;
    for (int i = 0; i < Q->size(); i++)
        Qt->add(i, new ListVal);

    for (int i = 0; i < Q->size(); i++)
    for (int j = 0; j < Q->size(); j++) {
        // Add to the transpose
        Val q = ((ListVal*) Q->get(j))->get(i);
        q->add_ref();
        ((ListVal*) Qt->get(i))->add(j, q);
    }
    
    // Upate with the true result
    Q->rem_ref();
    Q = Qt;
    
    // Finalize the result
    ListVal *QR = new ListVal;
    QR->add(0, Q);
    QR->add(1, R);

    return QR;

}

Val std_trace(Env env) {
    Val x = env->apply("x");

    if (!isVal<ListVal>(x)) {
        throw_err("type", "linalg.trace : [[R]] -> R cannot be applied to argument " + x->toString());
        return NULL;
    }

    ListVal *xss = (ListVal*) x;

    float tr = 0;
    
    for (int n = 0; n < xss->size(); n++) {
        // Check that the item is a list
        Val v = xss->get(n);
        if (!isVal<ListVal>(v)) {
            throw_err("type", "linalg.trace : [[R]] -> R cannot be applied to argument " + x->toString());
            return NULL;
        }
        
        // Acquire the row
        ListVal *xs = (ListVal*) v;
        if (n < xss->size()) {
            throw_err("type", "linalg.trace : [[R]] -> R cannot be applied to argument " + x->toString());
            return NULL;
        }

        // Get the item
        v = xs->get(n);
        
        // Process it
        if (isVal<IntVal>(v))
            tr += ((IntVal*) v)->get();
        else if (isVal<RealVal>(v))
            tr += ((RealVal*) v)->get();
        else {
            throw_err("type", "linalg.trace : [[R]] -> R cannot be applied to argument " + x->toString());
            return NULL;
        }
    }
   
    return new RealVal(tr);
}

Val std_gaussian(Env env) {
    Val x = env->apply("x");

    if (!isVal<ListVal>(x)) {
        throw_err("type", "linalg.gaussian : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
        return NULL;
    }

    ListVal *M = (ListVal*) x;
    
    // Should be a non-empty list
    int rows = M->size();
    if (rows == 0) {
        throw_err("type", "linalg.gaussian : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
        return NULL;
    }

    // We will ensure that M is rectangular and consisting exclusively of numbers.
    int cols;
    for (int i = 0; i < rows; i++) {
        Val row = M->get(i);
        if (!isVal<ListVal>(row)) {
            throw_err("type", "linalg.gaussian : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
            return NULL;
        }
        
        auto it = ((ListVal*) row)->iterator();
        int j = 0;
        while (it->hasNext()) {
            auto v = it->next();
            if (!isVal<RealVal>(v) && !isVal<IntVal>(v)) {
                delete it;
                throw_err("type", "linalg.gaussian : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
                return NULL;
            }
            j++;
        }
        delete it;

        if (!i) {
            if (!j) {
                throw_err("type", "linalg.gaussian : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
                return NULL;
            } else
                cols = j;
        } else if (cols != j) {
            throw_err("type", "linalg.gaussian : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
            return NULL;
        }
    }

    // We will build a 2D array
    float **mtrx = new float*[rows];
    for (int i = 0; i < rows; i++) {
        mtrx[i] = new float[cols];
        for (int j = 0; j < cols; j++) {
            Val v = ((ListVal*) M->get(i))->get(j);
            if (isVal<IntVal>(v))
                mtrx[i][j] = ((IntVal*) v)->get();
            else
                mtrx[i][j] = ((RealVal*) v)->get();
        }
    }

    int n = rows > cols ? cols : rows;
    
    int i;
    for (i = 0; i < n; i++) {
        int j = i;
        while (j < rows && mtrx[j][i] == 0)
            j++;
        
        // Swap rows if necessary
        if (j == rows)
            // Non-reducible
            break;
        if (i != j) {
            auto tmp = mtrx[i];
            mtrx[i] = mtrx[j];
            mtrx[j] = tmp;
        }
        
        // Normalize the row
        for (int j = i+1; j < cols; j++)
            mtrx[i][j] /= mtrx[i][i];
        mtrx[i][i] = 1;
        
        // Perform row subtractions
        for (int j = i+1; j < rows; j++) {
            for (int k = i+1; k < cols; k++)
                mtrx[j][k] -= mtrx[j][i] * mtrx[i][k];
            mtrx[j][i] = 0;
        }
    }
    
    // Reduce the upper triangle
    while (i--) {
        for (int j = i+1; j < cols; j++)
            mtrx[i][j] /= mtrx[i][i];
        mtrx[i][i] = 1;
        for (int j = 0; j < i; j++) {
            for (int k = i+1; k < cols; k++)
                mtrx[j][k] -= mtrx[j][i] * mtrx[i][k];
            mtrx[j][i] = 0;
        }
    }
    
    // Finalize the answer
    M = new ListVal;
    for (int i = 0; i < rows; i++) {
        auto row = new ListVal;
        M->add(i, row);

        for (int j = 0; j < cols; j++)
            row->add(j, new RealVal(mtrx[i][j]));
        
        delete[] mtrx[i];
    }
    delete[] mtrx;

    return M;
}


Val std_determinant(Env env) {
    Val x = env->apply("x");

    // Should be a square matrix
    int n = is_square_matrix(x);
    if (n == 0) {
        throw_err("type", "linalg.determinant : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
        return NULL;
    }
    
    // Acquire the matrix in list form.
    ListVal *M = (ListVal*) x;

    // We will build a 2D array
    float **mtrx = new float*[n];
    for (int i = 0; i < n; i++) {
        mtrx[i] = new float[n];
        for (int j = 0; j < n; j++) {
            Val v = ((ListVal*) M->get(i))->get(j);
            if (isVal<IntVal>(v))
                mtrx[i][j] = ((IntVal*) v)->get();
            else
                mtrx[i][j] = ((RealVal*) v)->get();
        }
    }
    
    // Initial condition: the determinant is 1
    float det = 1;
    
    int i;
    for (i = 0; i < n; i++) {

        int j = i;
        while (j < n && mtrx[j][i] == 0)
            j++;
        
        if (j == n) {
            // Non-invertible, therefore determinant is zero
            det = 0;
            break;
        } else if (i != j) {
            // Swap rows
            auto tmp = mtrx[i];
            mtrx[i] = mtrx[j];
            mtrx[j] = tmp;
            
            det *= -1; // On swap, negate determinant
        }
        
        // Normalize the row
        for (int j = i+1; j < n; j++)
            mtrx[i][j] /= mtrx[i][i];
        
        det *= mtrx[i][i];
        mtrx[i][i] = 1;
        
        // Perform row subtractions
        for (int j = i+1; j < n; j++) {
            for (int k = i+1; k < n; k++)
                mtrx[j][k] -= mtrx[j][i] * mtrx[i][k];
            mtrx[j][i] = 0;
        }
    }
    
    // Garbage collection
    for (int i = 0; i < n; i++)
        delete[] mtrx[i];
    delete[] mtrx;

    return new RealVal(det);

}

Val std_eig(Env env) {
    Val x = env->apply("x");

    int n = is_square_matrix(x);
    if (n == 0) {
        throw_err("type", "linalg.determinant : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
        return NULL;
    }
    
    // Acquire the matrix in list form.
    ListVal *A = (ListVal*) x; 

    float *c = characteristic_poly(A);

    // Then, we will find the solutions to the polynomial.
    float *eigs = factorize(c, n);

    auto res = new ListVal;
    for (int i = 0; i < n; i++) {
        res->add(i, new RealVal(eigs[i]));
    }
    
    delete[] c;
    delete[] eigs;
    
    return res;
}

Val std_characteristic_polynomial(Env env) {
    Val x = env->apply("x");

    int n = is_square_matrix(x);
    if (n == 0) {
        throw_err("type", "linalg.determinant : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
        return NULL;
    }
    
    // Acquire the matrix in list form.
    ListVal *A = (ListVal*) x; 

    float *c = characteristic_poly(A);
    
    // Construct the polynomial
    Exp poly = NULL;
    for (int i = 0; i <= n; i++) {
        if (c[i] != 0) {
            // Compute the element of the polynomial.
            Exp arg = i > 0
                    ? (Exp) new MultExp(
                        new RealExp(c[i]),
                        new ExponentExp(new VarExp("x"), new IntExp(i)))
                    : (Exp) new RealExp(c[i]);

            if (poly) {
                poly = new SumExp(arg, poly);
            } else {
                poly = arg;
            }
        }
    }

    if (!poly) {
        // Because all of the coefficients are zero, the polynomial should also yield zero.
        poly = new RealExp;
    }
    
    // Generate the function.
    return new LambdaVal(new std::string[2]{"x", ""}, poly);
}

Type* type_stdlib_linalg() {
    return new DictType {
        {
            "characteristic_polynomial",
            new LambdaType("x",
                new ListType(new ListType(new RealType)),
                new LambdaType("x", new RealType, new RealType))
        }, {
            "det",
            new LambdaType("x",
                new ListType(new ListType(new RealType)),
                new RealType)
        }, {
            "eig",
            new LambdaType("x",
                new ListType(new ListType(new RealType)),
                new ListType(new RealType))
        }, {
            "gaussian",
            new LambdaType("x",
                new ListType(new ListType(new RealType)),
                new ListType(new ListType(new RealType)))
        }, {
            "qr",
            new LambdaType("x",
                new ListType(new ListType(new RealType)),
                new ListType(new ListType(new ListType(new RealType))))
        }, {
            "trace",
            new LambdaType("x",
                new ListType(new ListType(new RealType)),
                new RealType)
        }, {
            "transpose",
            new LambdaType("x",
                new ListType(new ListType(new RealType)),
                new ListType(new ListType(new RealType)))
        }
    };
}

Val load_stdlib_linalg() {
    return new DictVal {
        {
            "characteristic_polynomial",
            new LambdaVal(new std::string[2]{"x", ""},
                new ImplementExp(std_characteristic_polynomial, NULL))
        },{
            "det",
            new LambdaVal(new std::string[2]{"x", ""},
                new ImplementExp(std_determinant, NULL))
        }, {
            "eig",
            new LambdaVal(new std::string[2]{"x", ""},
                new ImplementExp(std_eig, NULL))
        }, {
            "gaussian",
            new LambdaVal(new std::string[2]{"x", ""},
                new ImplementExp(std_gaussian, NULL))
        }, {
            "qr",
            new LambdaVal(new std::string[2]{"x", ""},
                new ImplementExp(std_qr, NULL))
        }, {
            "trace",
            new LambdaVal(new std::string[2]{"x", ""},
                new ImplementExp(std_trace, NULL))
        }, {
            "transpose",
            new LambdaVal(new std::string[2]{"x", ""},
                new ImplementExp(std_transpose, NULL))
        }
    };
}


