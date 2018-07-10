#include "optimize.hpp"
#include "expression.hpp"

using namespace std;

bool is_const_list(ListExp *exp) {
    bool res = true;

    auto it = exp->iterator();
    while (it->hasNext() && res)
        res = is_const_exp(it->next());

    delete it;

    return res;
}

inline bool is_const_exp(Exp exp) {
    if (isExp<ListExp>(exp))
        return is_const_list((ListExp*) exp);
    else return
        isExp<FalseExp>(exp) ||
        isExp<IntExp>(exp) ||
        isExp<RealExp>(exp) ||
        isExp<StringExp>(exp) ||
        isExp<TrueExp>(exp) ||
        isExp<VoidExp>(exp);
}

Exp FoldExp::optimize() {
    list = list->optimize();
    func = func->optimize();
    base = base->optimize();
    return this;
}
    
Exp ForExp::optimize() {
    set = set->optimize();
    body = body->optimize();

    if (isExp<ListExp>(set) && ((ListExp*) set)->size() == 0) {
        // If the list is always an empty list, then the body will never execute.
        delete this;
        return new VoidExp;
    } else
        return this;

}

Exp IfExp::optimize() {
    cond = cond->optimize();

    if (isExp<TrueExp>(cond)) {
        // Because the condition is always true, the if statement
        // is redundant. So, we drop all but the true expression.
        throw_debug("preprocessor", "simplified if-then-else conditional to true");

        auto res = tExp->optimize();
        tExp = NULL;

        delete this;

        return res;

    } else if (isExp<FalseExp>(cond)) {
        // Similarly, because the expression is always false, we
        // will simply drop the true body and the condition.
        throw_debug("preprocessor", "simplified if-then-else conditional to false");

        auto res = fExp->optimize();
        fExp = NULL;

        delete this;

        return res;

    } else {
        tExp = tExp->optimize();
        fExp = fExp->optimize();

        return this;
    }
}
    
Exp LetExp::optimize() {
    // For a let-exp, we seek to propagate constant variables through the
    // expression and replace them wherever possible. Then, we can allow
    // constant folding to further reduce.

    opt_varexp_map consts;
    opt_varexp_map ends;
    
    int i;
    for (i = 0; exps[i]; i++) {
        exps[i] = exps[i]->optimize();
        
        // If the optimal value is constant, then we should store
        // it for the purpose of constant propagation.
        if (is_const_exp(exps[i])) {
            throw_debug("postprocessor", "will attempt to propagate constant " + ids[i] + " as " + exps[i]->toString());
            consts[ids[i]] = exps[i]->clone();
            ends[ids[i]] = exps[i]->clone();
        }
    }
    
    // Then, we do plain optimization.
    body = body->optimize();

    if (is_const_exp(body)) {
        Exp e = body->clone();
        delete this;
        return e;
    } else {
        return this;
    }
}

Exp ListExp::optimize() {
    auto tmp = new ListExp;
    
    // Optimize each element one by one.
    while (!isEmpty())
        tmp->add(0, remove(0)->optimize());
    
    // Reassemble the list.
    while (!tmp->isEmpty())
        add(0, tmp->remove(0));

    return this;
}
    
Exp ListAccessExp::optimize() {
    list = list->optimize();
    idx = idx->optimize();

    if (isExp<ListExp>(list) && isExp<IntExp>(idx)) {
        int i = ((IntExp*) idx)->get();
        auto lst = ((ListExp*) list);
        if (i >= 0 && i < lst->size()) {
            Exp e = lst->remove(i);
            delete this;
            return e;
        } else {
            return this;
        }
    }
}

Exp NotExp::optimize() {
    exp = exp->optimize();

    if (isExp<TrueExp>(exp)) {
        // not true = false
        delete exp;
        return new FalseExp;
    } else if (isExp<FalseExp>(exp)) {
        // not false = true
        delete exp;
        return new TrueExp;
    } else
        return this;
}

Exp OperatorExp::optimize() {
    left = left->optimize();
    right = right->optimize();

    if (is_const_exp(left) && is_const_exp(right)) {
        // If the left and right hand side are constants, then we
        // can perform constant folding on the operation.
        Val v = evaluate(NULL);
        
        if (!v)
            return this;
        else {
            Exp e = new ValExp(v);
            v->rem_ref();
            return e;
        }
    } else
        return this;
}

Exp PrintExp::optimize() {
    int i;
    for (i = 0; args[i]; i++) {
        args[i] = args[i]->optimize();

        if (is_const_exp(args[i])) {
            // Precompute the string
            string s = args[i]->toString();
            delete args[i];
            args[i] = new StringExp(s);
        }
    }

    return this;
}

Exp DictExp::optimize() {
    auto vs = new LinkedList<Exp>;
    
    while (!vals->isEmpty()) {
        Exp v = vals->remove(0)->optimize();

        vs->add(vs->size(), v);
    }
    delete vals;

    vals = vs;

    return this;
}

Exp DictAccessExp::optimize() {
    return this;
}

Exp SetExp::optimize() {
    tgt->optimize();
    exp->optimize();
    return this;
}

Exp SequenceExp::optimize() {
    auto exps = new LinkedList<Exp>;

    while (!seq->isEmpty()) {
        Exp exp = seq->remove(0)->optimize();
        
        if (!is_const_exp(exp) || seq->isEmpty())
            exps->add(0, exp);
        else
            delete exp;
    }

    if (exps->size() == 1) {
        // If there is only one statement, that is all we need.
        Exp exp = exps->remove(0);
        delete exps; delete this;
        return exp;
    }

    while (!exps->isEmpty())
        seq->add(0, exps->remove(0));
    delete exps;
    
    return this;
}

Exp StdMathExp::optimize() {
    // Optimize the argument
    e = e->optimize();

    Exp res = this;
    
    // We will simplify certain interesting items
    switch (fn) {
        case LOG:
            // log(exp(x)) = x
            if (isExp<StdMathExp>(e) && ((StdMathExp*) e)->fn == EXP) {
                res = ((StdMathExp*) e)->e->clone();
                delete this;
            }
            break;
        case EXP: // exp(log(x)) = x
            if (isExp<StdMathExp>(e)) {
                auto x = (StdMathExp*) e;
                if (x->fn == LOG) {
                    res = x->e->clone();
                    delete this;
                }
            }
            break;
        case SQRT:
            if (isExp<StdMathExp>(e)) {
                auto x = (StdMathExp*) e;
                if (x->fn == EXP) {
                    // sqrt(exp(x)) = exp(x/2)
                    delete e;
                    fn = EXP;
                    e = new MultExp(new RealExp(0.5), e->clone());
                    return res->optimize();
                }
            }
            break;
    }

    return res;

}

Exp WhileExp::optimize() {
    cond = cond->optimize();

    if (isExp<FalseExp>(cond)) {
        // The condition is always false, hence it will never execute.
        delete this;
        return new VoidExp;
    } else {
        body = body->optimize();
        return this;
    }
}

