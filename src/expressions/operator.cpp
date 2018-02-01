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
            List<Val> *A = ((ListVal*) a)->get();
            List<Val> *B = ((ListVal*) b)->get();
            if (A->size() != B->size()) {
                throw_err("runtime", "cannot subtract lists '" + left->toString() + "' and '" + right->toString() + "' of differing lengths");
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

    } else if (typeid(*a) == typeid(VoidVal) && typeid(*b) == typeid(VoidVal)) {
        return new BoolVal(operation == CompOp::EQ);
    } else {
        throw_err("runtime", "division is not defined between " + left->toString() + " and " + right->toString());
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
    if (!lst) return NULL;
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

Val pure_dot_table(Val a, Val b) {
    MultExp mult(NULL, NULL);
    SumExp sum(NULL, NULL);

    if (typeid(*a) == typeid(ListVal)) {
        Val v = NULL;

        auto ait = ((ListVal*) a)->get()->iterator();
        auto bit = ((ListVal*) b)->get()->iterator();

        if (!ait->hasNext() || !bit->hasNext()) {
            delete ait;
            delete bit;
            delete v;
            return NULL;
        }

        do {
            // Proceed through the next step of the computation
            if (v)
                v = sum.op(v, pure_dot_table(ait->next(), bit->next()));
            else
                v = pure_dot_table(ait->next(), bit->next());

        } while (v && ait->hasNext() && bit->hasNext());
        
        if (v && (ait->hasNext() || bit->hasNext())) {
            v->rem_ref();
            v = NULL;
        }

        delete ait;
        delete bit;

        return v;

    } else
        return mult.op(a, b);

    return NULL;
}

Val dot_table(ListVal *a, ListVal *b, int order) {
    
    if (order > 0) {
        //std::cout << "dot table btwn " << *a << " and " << *b << "\n";
        // Left heavy order-wise
        ListVal *tensor = new ListVal;

        auto rit = a->get()->iterator();
        while (rit->hasNext()) {
            ListVal *r = (ListVal*) rit->next();

            Val v = order > 1
                        ? dot_table(r, b, order - 1)
                        : pure_dot_table(r, b);

            if (!v) {
                tensor->rem_ref();
                delete rit;
                return NULL;
            }

            tensor->get()->add(tensor->get()->size(), v);
        }
        delete rit;

        //if (tensor) std::cout << "dot table: " << *tensor << "\n";

        return tensor;

    } else if (order < 0) {
        //std::cout << "transpose dot table btwn " << *a << " and " << *b << "\n";

        // Right heavy order-wise
        ListVal *aT = transpose(a);
        ListVal *bT = transpose(b);

        Val cT = dot_table(bT, aT, -order);
        aT->rem_ref();
        bT->rem_ref();

        if (!cT) return NULL;

        Val c = transpose((ListVal*) cT);
        cT->rem_ref();

        //if (c) std::cout << "transpose dot table: " << *c << "\n";

        return c;

    } else {
        /*
        if (a->get()->size() != b->get()->size())
            return NULL;
        */

        //std::cout << "pure dot table btwn " << *a << " and " << *b << "\n";

        ListVal *tensor = new ListVal;

        auto rit = a->get()->iterator();

        while (rit->hasNext()) {
            Val r = rit->next();

            ListVal *row = new ListVal;
            tensor->get()->add(tensor->get()->size(), row);

            auto cit = b->get()->iterator();
            while (cit->hasNext()) {
                Val c = cit->next();

                Val v = pure_dot_table(r, c);

                if (!v) {
                    tensor->rem_ref();
                    delete rit;
                    delete cit;
                    return NULL;
                }

                row->get()->add(row->get()->size(), v);
            }

            delete cit;

        }
        
        delete rit;

        //if (tensor) std::cout << "pure dot table: " << *tensor << "\n";

        return tensor;
    }
}

// Expression for multiplying studd
Val MultExp::op(Value *a, Value *b) {

    if (typeid(*a) == typeid(ListVal)) {

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

            int ordA = 0, ordB = 0;
            for (Val A = a; typeid(*A) == typeid(ListVal); ordA++)
                A = ((ListVal*) A)->get()->get(0);
            for (Val B = b; typeid(*B) == typeid(ListVal); ordB++)
                B = ((ListVal*) B)->get()->get(0);

            int ord = ordA - ordB;
            
            Val res = dot_table((ListVal*) a, bT, ord);
            bT->rem_ref();

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
        throw_err("runtime", "multiplication is not defined between " + left->toString() + " and " + right->toString());
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
