#include "expressions/operator.hpp"

#include "value.hpp"

#include <string>

using namespace std;

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
    }
    if (typeid(*b) == typeid(BoolVal) ||
        typeid(*b) == typeid(LambdaVal)) {
        throw_err("runtime", "subtraction is not defined on lambdas or booleans\nsee:\n" + right->toString());
        return NULL;
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

Value* EqualsExp::op(Value *a, Value *b) {

    // If the two are physically identical, they are equal
    if (a == b)
        return new BoolVal(true);
   
    // Lambdas cannot be compared
    if (typeid(*a) == typeid(LambdaVal)) {
        throw_warning("runtime", "equality checking is not defined on lambdas\nsee:\n" + left->toString());
        return new BoolVal(false);
    } else if(typeid(*b) == typeid(LambdaVal)) {
        throw_warning("runtime", "equality checking is not defined on lambdas\nsee:\n" + right->toString());
        return new BoolVal(false);
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

    return new BoolVal(A == B);
}

// Expression for multiplying studd
Value* MultExp::op(Value *a, Value *b) {
    
    if (typeid(*a) == typeid(BoolVal) ||
        typeid(*a) == typeid(LambdaVal)) {
        throw_err("runtime", "multiplication is not defined on lambdas or booleans\nsee:\n" + left->toString());
        return NULL;
    }
    if (typeid(*b) == typeid(BoolVal) ||
        typeid(*b) == typeid(LambdaVal)) {
        throw_err("runtime", "multiplication is not defined on lambdas or booleans\nsee:\n" + right->toString());
        return NULL;
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
    auto z = x * y;
    if (typeid(*a) == typeid(RealVal) || typeid(*b) == typeid(RealVal))
        return new RealVal(z);
    else
        return new IntVal(z);

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
    if (typeid(*a) == typeid(ListVal) &&
        typeid(*b) == typeid(ListVal)) {
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

