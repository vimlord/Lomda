#include "expressions/operator.hpp"

#include "value.hpp"
#include "config.hpp"
#include "types.hpp"

#include <string>

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
                throw_err("runtime", "cannot subtract lists '" + left->toString() + "' and '" + right->toString() + "' of differing lengths");
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
        throw_err("runtime", "subtraction is not defined between " + left->toString() + " and " + right->toString());
        return NULL;
    }

}

// Expression for multiplying studd
Val DivExp::op(Value *a, Value *b) {

    if (val_is_number(a) && val_is_number(b)) {
        auto x = val_is_integer(a) ? ((IntVal*) a)->get() : ((RealVal*) a)->get();
        auto y = val_is_integer(b) ? ((IntVal*) b)->get() : ((RealVal*) b)->get();

        // Compute the result
        auto z = x / y;
        if (typeid(*a) == typeid(RealVal) || typeid(*b) == typeid(RealVal))
            return new RealVal(z);
        else
            return new IntVal(z);
    } else {
        throw_err("runtime", "division is not defined between " + left->toString() + " and " + right->toString());
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
            std::cout << "compute " << *a << " * " << *b << "\n";

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
                    std::cout << "2 by 1\n";
                    // Matrix by vector
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
                } else {
                    // Matrix by matrix
                    std::cout << "2 by 2\n";
                    
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
                std::cout << "1 by 2\n";
                // List by matrix
                auto ait = ((ListVal*) a)->get()->iterator();
                auto bit = ((ListVal*) b)->get()->iterator();

                MultExp mult(NULL, NULL);
                SumExp sum(NULL, NULL);

                res = mult.op(ait->next(), bit->next());
                if (res)
                    std::cout << "init: " << *res << "\n";
                
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
                std::cout << "1 by 1\n";
                // Dot product
                res = new IntVal;
                
                auto ait = ((ListVal*) a)->get()->iterator();
                auto bit = ((ListVal*) a)->get()->iterator();

                SumExp sum(NULL, NULL);
                
                while (res && ait->hasNext() && bit->hasNext()) {
                    Val v = op(ait->next(), bit->next());
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
                throw_err("runtime", "cannot add lists '" + left->toString() + "' and '" + right->toString() + "' of differing lengths");
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
        throw_err("runtime", "addition is not defined between " + left->toString() + " and " + right->toString());
        return NULL;
    }

}
