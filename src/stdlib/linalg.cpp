#include "stdlib.hpp"

#include "expressions/stdlib.hpp"
#include "expression.hpp"

Val std_transpose(Env env) {
    Val x = env->apply("x");

    if (!isVal<ListVal>(x)) {
        throw_err("type", "linalg.transpose : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
        return NULL;
    }

    ListVal *xss = (ListVal*) x;

    int rows = 0, cols = 0;
    
    auto it = xss->get()->iterator();
    while (it->hasNext()) {
        Val xs = it->next();
        if (!isVal<ListVal>(xs)) {
            throw_err("type", "linalg.transpose : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
            delete it;
            return NULL;
        }

        int c = 0;
        auto jt = ((ListVal*) xs)->get()->iterator();
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
        yss->get()->add(i, ys);

        // Add the row transpose
        for (int j = 0; j < rows; j++) {
            ListVal *row = (ListVal*) (xss->get()->get(j));
            Val y = row->get()->get(i);
            ys->get()->add(j, y);
            y->add_ref();
        }
    }

    return yss;

}

Val std_gaussian(Env env) {
    Val x = env->apply("x");

    if (!isVal<ListVal>(x)) {
        throw_err("type", "linalg.gaussian : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
        return NULL;
    }

    ListVal *M = (ListVal*) x;
    
    // Should be a non-empty list
    int rows = M->get()->size();
    if (rows == 0) {
        throw_err("type", "linalg.gaussian : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
        return NULL;
    }

    // We will ensure that M is rectangular and consisting exclusively of numbers.
    int cols;
    for (int i = 0; i < rows; i++) {
        Val row = M->get()->get(i);
        if (!isVal<ListVal>(row)) {
            throw_err("type", "linalg.gaussian : [[R]] -> [[R]] cannot be applied to argument " + x->toString());
            return NULL;
        }
        
        auto it = ((ListVal*) row)->get()->iterator();
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
            Val v = ((ListVal*) M->get()->get(i))->get()->get(j);
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
        M->get()->add(i, row);

        for (int j = 0; j < cols; j++)
            row->get()->add(j, new RealVal(mtrx[i][j]));
        
        delete[] mtrx[i];
    }
    delete[] mtrx;

    return M;

}

Type* type_stdlib_linalg() {
    return new DictType {
        {
            "gaussian",
            new LambdaType("x",
                new ListType(new ListType(new RealType)),
                new ListType(new ListType(new RealType)))
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
            "gaussian",
            new LambdaVal(new std::string[2]{"x", ""},
                new ImplementExp(std_gaussian, NULL))
        }, {
            "transpose",
            new LambdaVal(new std::string[2]{"x", ""},
                new ImplementExp(std_transpose, NULL))
        }
    };
}


