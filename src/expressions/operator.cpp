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
Val AndExp::op(Value *a, Value *b) {
    
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
Val DiffExp::op(Value *a, Value *b) {
    
    if (typeid(*a) == typeid(BoolVal) ||
        typeid(*a) == typeid(LambdaVal)) {
        throw_err("runtime", "subtraction is not defined on lambdas or booleans\nsee:\n" + left->toString());
        return NULL;
    } else if (typeid(*b) == typeid(BoolVal) ||
        typeid(*b) == typeid(LambdaVal)) {
        throw_err("runtime", "subtraction is not defined on lambdas or booleans\nsee:\n" + right->toString());
        return NULL;
    } else if (typeid(*a) == typeid(ListVal)) {
        if (typeid(*b) == typeid(ListVal)) {
            // Concatenate the two lists
            List<Val> *A = ((ListVal*) a)->get();
            List<Val> *B = ((ListVal*) b)->get();
            if (A->size() != B->size()) {
                throw_err("runtime", "cannot add lists '" + left->toString() + "' and '" + right->toString() + "' of differing lengths");
                return NULL;
            }

            List<Val> *C = new LinkedList<Val>;
            
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
            throw_err("runtime", "type of '" + l->toString() + "' and '" + r->toString() + "' do not properly match");
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
Val DivExp::op(Value *a, Value *b) {
    
    if (typeid(*a) == typeid(BoolVal) ||
        typeid(*a) == typeid(LambdaVal)) {
        throw_err("runtime", "division is not defined on lambdas or booleans\nsee:\n" + left->toString());
        return NULL;
    } else if (typeid(*b) == typeid(BoolVal) ||
        typeid(*b) == typeid(LambdaVal)) {
        throw_err("runtime", "division is not defined on lambdas or booleans\nsee:\n" + right->toString());
        return NULL;
    }
    
    // The lhs is numerical
    auto x = 
        typeid(*a) == typeid(IntVal)
        ? ((IntVal*) a)->get() :
          ((RealVal*) a)->get();
    
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

Val CompareExp::op(Value *a, Value *b) { 

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

bool is_rect_tensor(ListVal *lst) {
    if (!lst) return false;
    else if (typeid(*lst->get()->get(0)) == typeid(ListVal)) {
        int size = 0;
        auto it = lst->get()->iterator();
        while (it->hasNext()) {
            Val v = it->next();
            if (!size) size = ((ListVal*) v)->get()->size();
            else if (typeid(*v) != typeid(ListVal) || ((ListVal*) v)->get()->size() != size) {
                delete it;
                return false;
            }
        }
        delete it;
        
        // Subtensors should be rectangular
        bool res = true;
        it = lst->get()->iterator();
        while (res && it->hasNext()) {
            ListVal *v = (ListVal*) it->next();
            res = is_rect_tensor(v);
        }
        delete it;
        
        // Subtensors should have matching sizes
        it = lst->get()->iterator();
        Val u = it->next();
        while (res && it->hasNext()) {
            Val v = it->next();
            Val w = u;
            while (typeid(*v) == typeid(ListVal) && typeid(*w) == typeid(ListVal)) {
                v = ((ListVal*) v)->get()->get(0);
                w = ((ListVal*) w)->get()->get(0);
            }

            res = ((typeid(*v) == typeid(ListVal)) == (typeid(*w) == typeid(ListVal)));
        }
        delete it;

        return res;
    } else {
        return true;
    }
}

ListVal* nd_array(int *size, int d, int x = 0) {
    if (x == d) return NULL;
    
    ListVal *lst = new ListVal;

    for (int i = 0; i < size[d-x-1]; i++) {
        lst->get()->add(0, nd_array(size, d, x+1));
    }
    
    return lst;

}

ListVal* transpose(ListVal *lst) {
    if (!is_rect_tensor(lst)) {
        throw_err("runtime", "tensor defined by '" + lst->toString() + "' is not properly formed");
        return NULL;
    }

    int order = 0;
    Val v = lst;
    while (typeid(*v) == typeid(ListVal)) {
        order++;
        v = ((ListVal*) v)->get()->get(0);
    }

    int *dim = new int[order];
    int *idx = new int[order];

    v = lst;
    for (int i = 0; i < order; i++) {
        dim[i] = ((ListVal*) v)->get()->size();
        idx[i] = 0;

        v = ((ListVal*) v)->get()->get(0);
    }

    ListVal *res = nd_array(dim, order);

    while (idx[order-1] < dim[0]) {
        ListVal *u = lst;
        ListVal *v = res;
        
        for (int i = order-1; i > 0; i--) {
            u = (ListVal*) u->get()->get(idx[i]);
            v = (ListVal*) v->get()->get(idx[order-1-i]);
        }
        
        Val x = u->get()->get(idx[0]);
        x->add_ref();
        v->get()->set(idx[order-1], x);

        idx[0] += 1;
        for (int i = 0; idx[i] == dim[order-1-i] && i < order-1; i++) {
            idx[i] = 0;
            idx[i+1]++;
        }
    }

    delete dim;
    delete idx;

    return res;
}

Val dot_table(ListVal *a, ListVal *b) {

    if (typeid(*(b->get()->get(0))) != typeid(ListVal)) {

        MultExp mult(NULL, NULL);
        SumExp sum(NULL, NULL);

        auto rit = a->get()->iterator();

        if (typeid(*(a->get()->get(0))) == typeid(ListVal)) {
            //std::cout << "left dot table between " << *a << " and " << *b << "\n";
            ListVal *tensor = new ListVal;

            while (rit->hasNext()) {
                ListVal *r = (ListVal*) rit->next();

                auto ait = r->get()->iterator();
                auto bit = ((ListVal*) b)->get()->iterator();

                Val v = mult.op(ait->next(), bit->next());

                while (v && ait->hasNext()) {
                    Val u = mult.op(ait->next(), bit->next());
                    if (u) {
                        Val w = sum.op(v, u);

                        u->rem_ref();
                        v->rem_ref();
                        
                        v = w ? w : NULL;

                    } else {
                        v->rem_ref();
                        v = NULL;
                    }
                }
                delete ait;
                delete bit;
                
                if (v)
                    tensor->get()->add(tensor->get()->size(), v);
                else {
                    delete rit;
                    delete tensor;
                    return NULL;
                }
            }

            delete rit;

            return tensor;
        
        } else {
            //std::cout << "pure dot table between " << *a << " and " << *b << "\n";


            auto bit = ((ListVal*) b)->get()->iterator();

            Val v = mult.op(rit->next(), bit->next());

            while (v && bit->hasNext() && rit->hasNext()) {
                Val R = rit->next();
                Val B = bit->next();

                Val u = mult.op(R, B);
                if (u) {
                    Val w = sum.op(v, u);
                    
                    u->rem_ref();
                    v->rem_ref();

                    v = w;

                } else {
                    delete rit;
                    delete bit;

                    v->rem_ref();
                    v = NULL;

                    return NULL;
                }
            }

            if (rit->hasNext() || bit->hasNext()) {
                throw_err("runtime", "pure dot product between '" + a->toString() + "' and '" + b->toString() + "' is not defined");
                v->rem_ref();
                v = NULL;
            }

            delete bit;
            delete rit;

            return v;
        }
        
    } else if (typeid(*(a->get()->get(0))) == typeid(ListVal)) {
        //std::cout << "non-reduced dot table between " << *a << " and " << *b << "\n";

        // 2+d by 2+d
        ListVal *tensor = new ListVal;
        auto rit = a->get()->iterator();
        while (rit->hasNext()) {
            ListVal *r = (ListVal*) rit->next();

            ListVal *row = new ListVal;
            tensor->get()->add(tensor->get()->size(), row);

            auto cit = b->get()->iterator();
            while (cit->hasNext()) {
                ListVal *c = (ListVal*) cit->next();

                Val v = dot_table(r, c);
                if (v)
                    row->get()->add(row->get()->size(), v);
                else {
                    delete rit;
                    delete cit;
                    delete tensor;
                    return NULL;
                }
            }
            delete cit;
        }
        delete rit;

        return tensor;

    } else {
        //std::cout << "right dot table between " << *a << " and " << *b << "\n";

        MultExp mult(NULL, NULL);
        SumExp sum(NULL, NULL);

        auto rit = b->get()->iterator();
        auto ait = a->get()->iterator();

        Val tensor = mult.op(ait->next(), rit->next());

        while (tensor && rit->hasNext()) {
            Val r = (ListVal*) rit->next();
            Val x = ait->next();

            Val v = mult.op(x, r);
            if (v) {
                Val w = sum.op(tensor, v);
                
                v->rem_ref();

                tensor = w ? w : NULL;

            } else {
                tensor->rem_ref();
                tensor = NULL;
            }
        }
        delete ait;
        delete rit;
        
        return tensor;
    }
}

// Expression for multiplying studd
Val MultExp::op(Value *a, Value *b) {

    if (typeid(*a) == typeid(BoolVal) ||
        typeid(*a) == typeid(LambdaVal)) {
        throw_err("runtime", "multiplication is not defined on lambdas or booleans\nsee:\n" + left->toString());
        return NULL;
    } else if (typeid(*b) == typeid(BoolVal) ||
        typeid(*b) == typeid(LambdaVal)) {
        throw_err("runtime", "multiplication is not defined on lambdas or booleans\nsee:\n" + right->toString());
        return NULL;
    } else if (typeid(*a) == typeid(ListVal)) {

        if (typeid(*b) == typeid(ListVal)) {
            ListVal *bT = transpose((ListVal*) b);
            
            if (!bT)
                return NULL;
            else if (!is_rect_tensor((ListVal*) a)) {
                throw_err("runtime", "multiplication is not defined on non-rectangular lists\nsee:\n" + left->toString() + "'");
                bT->rem_ref();
                return NULL;
            }

            //std::cout << "to compute " << *a << " * " << *b << ", we assemble dot from right:\ndot list: " << *bT << "\n";
            
            Val res = dot_table((ListVal*) a, bT);

            bT->rem_ref();

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
    }

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
}

// Expression for adding stuff
Val SumExp::op(Value *a, Value *b) {
    // Handle strings
    if (typeid(*a) == typeid(StringVal) ||
        typeid(*b) == typeid(StringVal))
        return new StringVal(a->toString() + b->toString());

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
            List<Val> *A = ((ListVal*) a)->get();
            List<Val> *B = ((ListVal*) b)->get();
            if (A->size() != B->size()) {
                if (left && right)
                    throw_err("runtime", "cannot add lists '" + left->toString() + "' and '" + right->toString() + "' of differing lengths");
                else
                    throw_err("runtime", "cannot add lists of differing lengths");
                return NULL;
            }

            List<Val> *C = new LinkedList<Val>;
            
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
        } else {
            Stringable *l = left ? (Stringable*) left : (Stringable*) a;
            Stringable *r = right ? (Stringable*) right : (Stringable*) b;
            throw_err("runtime", "type of '" + l->toString() + "' and '" + r->toString() + "' do not properly match");
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

