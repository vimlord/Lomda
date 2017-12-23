#include "expressions/operator.hpp"

#include "value.hpp"
#include "config.hpp"

#include <string>

using namespace std;

OperatorExp::~OperatorExp() {
    delete left;
    delete right;
}

// Expression for adding stuff
Value* AndExp::op(Value *a, Value *b) {
    
    if (typeid(*a) != typeid(BoolVal)) {
        throw_type_err(left, "boolean");
        return NULL;
    }
    if (typeid(*b) != typeid(BoolVal)) {
        throw_type_err(right, "boolean");
        return NULL;
    }

    auto x = ((BoolVal*) a)->get();

    auto y = ((BoolVal*) b)->get();
    
    // Compute the result
    auto z = x && y;
    return new BoolVal(z);

}

// Expression for multiplying studd
Value* DiffExp::op(Value *a, Value *b) {
    
    if (typeid(*a) == typeid(BoolVal) ||
        typeid(*a) == typeid(LambdaVal)) {
        throw_err("runtime", "subtraction is not defined on lambdas or booleans\nsee:\n" + left->toString());
        return NULL;
    } else if (typeid(*b) == typeid(BoolVal) ||
        typeid(*b) == typeid(LambdaVal)) {
        throw_err("runtime", "subtraction is not defined on lambdas or booleans\nsee:\n" + right->toString());
        return NULL;
    } else if (typeid(*a) == typeid(MatrixVal)) {
        if (typeid(*b) == typeid(MatrixVal)) {
            Matrix A = ((MatrixVal*) a)->get();
            Matrix B = ((MatrixVal*) b)->get();
            
            if (A.R == B.R && A.C == B.C)
                return new MatrixVal(A-B);
            else {
                throw_err("runtime", "subtraction is not defined between " + to_string(A.R) + "x" + to_string(A.C) + "matrix and " + to_string(B.R) + "x" + to_string(B.C) + " matrix");
                return NULL;
            }
        } else {
            throw_err("runtime", "type of '" + left->toString() + "' and '" + right->toString() + "' do not properly match\n");
            return NULL;
        }
    }

    auto x = 
        typeid(*a) == typeid(IntVal)
        ? ((IntVal*) a)->get() :
          ((RealVal*) a)->get();

    auto y = 
        typeid(*b) == typeid(IntVal)
        ? ((IntVal*) b)->get() :
          ((RealVal*) b)->get();

    // Compute the result
    auto z = x - y;
    if (typeid(*a) == typeid(RealVal) || typeid(*b) == typeid(RealVal))
        return new RealVal(z);
    else
        return new IntVal(z);

}

// Expression for multiplying studd
Value* DivExp::op(Value *a, Value *b) {
    
    if (typeid(*a) == typeid(BoolVal) ||
        typeid(*a) == typeid(LambdaVal)) {
        throw_err("runtime", "division is not defined on lambdas or booleans\nsee:\n" + left->toString());
        return NULL;
    } else if (typeid(*b) == typeid(BoolVal) ||
        typeid(*b) == typeid(LambdaVal)) {
        throw_err("runtime", "division is not defined on lambdas or booleans\nsee:\n" + right->toString());
        return NULL;
    } else if (typeid(*a) == typeid(MatrixVal)) {
        Matrix A = ((MatrixVal*) a)->get();

        if (typeid(*b) == typeid(MatrixVal)) {
            Matrix B = ((MatrixVal*) b)->get();
            
            if (B.R != B.C) {
                throw_err("runtime", "non-square matrix '" + b->toString() + "' cannot be inverted");
                return NULL;
            } else if (A.C != B.R) {
                throw_err("runtime", "division is not defined between " + to_string(A.R) + "x" + to_string(A.C) + "matrix and " + to_string(B.R) + "x" + to_string(B.C) + " matrix");
                return NULL;
            } else return new MatrixVal(A/B);

        } else if (typeid(*b) == typeid(IntVal) || typeid(*b) == typeid(RealVal)) {
            return new MatrixVal(A/(typeid(*b) == typeid(IntVal) ? ((IntVal*) b)->get() : ((RealVal*) b)->get()));
        } else {
            throw_err("runtime", "division is not defined between '" + a->toString() + "' and '" + b->toString() + "'");
            return NULL;
        }
    }
    
    // The lhs is numerical
    auto x = 
        typeid(*a) == typeid(IntVal)
        ? ((IntVal*) a)->get() :
          ((RealVal*) a)->get();
    
    if (typeid(*b) == typeid(MatrixVal)) {
        if (((MatrixVal*) b)->get().R != ((MatrixVal*) b)->get().C) {
            throw_err("runtime", "non-square matrix '" + b->toString() + "' cannot be inverted");
            return NULL;
        } else {
            // rhs is a matrix
            Matrix M = ((MatrixVal*) b)->get();
            return new MatrixVal(x/M);
        }
    } else {
        // rhs is numerical
        auto y = 
            typeid(*b) == typeid(IntVal)
            ? ((IntVal*) b)->get() :
              ((RealVal*) b)->get();

        // Compute the result
        auto z = x / y;
        if (typeid(*a) == typeid(RealVal) || typeid(*b) == typeid(RealVal))
            return new RealVal(z);
        else
            return new IntVal(z);
    }
}

Value* CompareExp::op(Value *a, Value *b) { 

    // Lambdas cannot be compared
    if (typeid(*a) == typeid(LambdaVal)) {
        throw_err("runtime", "equality checking is not defined on lambdas\nsee:\n" + left->toString());
        return NULL;
    } else if(typeid(*b) == typeid(LambdaVal)) {
        throw_err("runtime", "equality checking is not defined on lambdas\nsee:\n" + right->toString());
        return NULL;
    }

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
}

// Expression for multiplying studd
Value* MultExp::op(Value *a, Value *b) {
    
    if (typeid(*a) == typeid(BoolVal) ||
        typeid(*a) == typeid(LambdaVal)) {
        throw_err("runtime", "multiplication is not defined on lambdas or booleans\nsee:\n" + left->toString());
        return NULL;
    } else if (typeid(*b) == typeid(BoolVal) ||
        typeid(*b) == typeid(LambdaVal)) {
        throw_err("runtime", "multiplication is not defined on lambdas or booleans\nsee:\n" + right->toString());
        return NULL;
    } else if (typeid(*a) == typeid(MatrixVal)) {
        Matrix A = ((MatrixVal*) a)->get();

        if (typeid(*b) == typeid(MatrixVal)) {
            Matrix B = ((MatrixVal*) b)->get();
            
            if (A.C != B.R) {
                throw_err("runtime", "multiplication is not defined between " + to_string(A.R) + "x" + to_string(A.C) + "matrix and " + to_string(B.R) + "x" + to_string(B.C) + " matrix");
                return NULL;
            } else return new MatrixVal(A*B);

        } else if (typeid(*b) == typeid(IntVal) || typeid(*b) == typeid(RealVal)) {
            return new MatrixVal(A*(typeid(*b) == typeid(IntVal) ? ((IntVal*) b)->get() : ((RealVal*) b)->get()));
        } else {
            throw_err("runtime", "multiplication is not defined between '" + a->toString() + "' and '" + b->toString() + "'");
            return NULL;
        }
    }
    
    // The lhs is numerical
    auto x = 
        typeid(*a) == typeid(IntVal)
        ? ((IntVal*) a)->get() :
          ((RealVal*) a)->get();
    
    if (typeid(*b) == typeid(MatrixVal)) {
        // rhs is a matrix
        return new MatrixVal(x*((MatrixVal*) b)->get());
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
}

// Expression for adding stuff
Value* SumExp::op(Value *a, Value *b) {

    if (typeid(*a) == typeid(BoolVal) ||
        typeid(*a) == typeid(LambdaVal)) {
        throw_err("runtime", "addition is not defined on lambdas or booleans\nsee:\n" + left->toString());
        return NULL;
    }
    if (typeid(*b) == typeid(BoolVal) ||
        typeid(*b) == typeid(LambdaVal)) {
        throw_err("runtime", "addition is not defined on lambdas or booleans\nsee:\n" + right->toString());
        return NULL;
    }
    if (typeid(*a) == typeid(ListVal)) {
        if (typeid(*b) == typeid(ListVal)) {
            // Concatenate the two lists
            List<Value*> *A = ((ListVal*) a)->get();
            List<Value*> *B = ((ListVal*) b)->get();
            List<Value*> *C = new LinkedList<Value*>;
            
            auto it = B->iterator();
            for (int i = 0; it->hasNext(); i++) C->add(i, it->next());
            delete it;
            it = A->iterator();
            for (int i = 0; it->hasNext(); i++) C->add(i, it->next());
            delete it;

            return new ListVal(C);
        } else {
            throw_err("runtime", "type of '" + left->toString() + "' and '" + right->toString() + "' do not properly match\n");
            return NULL;
        }
    } else if (typeid(*a) == typeid(MatrixVal)) {
        if (typeid(*b) == typeid(MatrixVal)) {
            Matrix A = ((MatrixVal*) a)->get();
            Matrix B = ((MatrixVal*) b)->get();
            
            if (A.R == B.R && A.C == B.C)
                return new MatrixVal(A+B);
            else {
                throw_err("runtime", "addition is not defined between " + to_string(A.R) + "x" + to_string(A.C) + "matrix and " + to_string(B.R) + "x" + to_string(B.C) + " matrix");
                return NULL;
            }
        } else {
            throw_err("runtime", "type of '" + left->toString() + "' and '" + right->toString() + "' do not properly match\n");
            return NULL;
        }
    }

    auto x = 
        typeid(*a) == typeid(IntVal)
        ? ((IntVal*) a)->get() :
          ((RealVal*) a)->get();

    auto y = 
        typeid(*b) == typeid(IntVal)
        ? ((IntVal*) b)->get() :
          ((RealVal*) b)->get();

    // Compute the result
    auto z = x + y;
    if (typeid(*a) == typeid(RealVal) || typeid(*b) == typeid(RealVal))
        return new RealVal(z);
    else
        return new IntVal(z);

}

