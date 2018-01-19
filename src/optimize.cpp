#include "optimize.hpp"
#include "expression.hpp"

using namespace std;

bool is_const_list(ListExp *exp) {
    bool res = true;

    auto it = exp->getList()->iterator();
    while (it->hasNext() && res)
        res = is_const_exp(it->next());

    delete it;

    return res;
}

inline bool is_const_exp(Exp exp) {
    
    if (typeid(*exp) == typeid(ListExp))
        return is_const_list((ListExp*) exp);

    return
        typeid(*exp) == typeid(FalseExp) ||
        typeid(*exp) == typeid(IntExp) ||
        typeid(*exp) == typeid(RealExp) ||
        typeid(*exp) == typeid(StringExp) ||
        typeid(*exp) == typeid(TrueExp) ||
        typeid(*exp) == typeid(VoidExp);
}
int ApplyExp::opt_var_usage(string x) {
    // The initial use is verifiable if it is a lambda-exp.
    // Otherwise, we assume that the variable will be used.
    int use = typeid(*op) == typeid(LambdaExp)
            ? op->opt_var_usage(x)
            : 3;

    for (int i = 0; args[i] && (use ^ 3); i++)
        use |= args[i]->opt_var_usage(x);

    return use;
}

Exp FoldExp::optimize() {
    list = list->optimize();
    func = func->optimize();
    base = base->optimize();
    return this;
}
Exp FoldExp::opt_const_prop(opt_varexp_map &vs, opt_varexp_map &ends) {
    list = list->opt_const_prop(vs, ends);
    base = base->opt_const_prop(vs, ends);
    
    if (typeid(*func) == typeid(ListExp) && ((ListExp*) list)->getList()->size() == 0) {
        // map [] into f = [] forall f
        Exp e = list; list = NULL;
        delete this;
        return e;
    } if (typeid(*func) == typeid(LambdaExp)) {
        // We can see if the function has side effects
        for (auto x : vs) {
            if (func->opt_var_usage(x.first) & 1) {
                delete vs[x.first];
                vs.erase(x.first);
            }
        }
        
        // Now that any variables chosen will not be affected, we can
        // simply proceed with propagation.
        func = func->opt_const_prop(vs, ends);

        return this;
    } else {
        // No assumptions can be made about the usage of the variables.
        // So, we must drop all of them.
        for (auto x : vs) {
            delete vs[x.first];
            vs.erase(x.first);
        }

        return this;
    }
}
int FoldExp::opt_var_usage(string x) {
    return func->opt_var_usage(x)
        | list->opt_var_usage(x)
        | base->opt_var_usage(x);
}

Exp ForExp::optimize() {
    set = set->optimize();
    body = body->optimize();

    if (typeid(*set) == typeid(ListExp) && ((ListExp*) set)->getList()->size() == 0) {
        // If the list is always an empty list, then the body will never execute.
        delete this;
        return new VoidExp;
    } else
        return this;
}
Exp ForExp::opt_const_prop(opt_varexp_map &vs, opt_varexp_map &ends) {
    set = set->opt_const_prop(vs, ends);
    
    // Now, we filter out the modified variables.
    for (auto x : vs) {
        if (body->opt_var_usage(x.first) & 1) {
            delete vs[x.first];
            vs.erase(x.first);
        }
    }

    body = body->opt_const_prop(vs, ends);

    return this;

}
int ForExp::opt_var_usage(string x) {
    if (id == x) return 0;
    else return set->opt_var_usage(x) | body->opt_var_usage(x);
}

Exp IfExp::optimize() {
    cond = cond->optimize();

    if (typeid(*cond) == typeid(TrueExp)) {
        // Because the condition is always true, the if statement
        // is redundant. So, we drop all but the true expression.
        throw_debug("preprocessor", "simplified if-then-else conditional to true");

        auto res = tExp->optimize();
        tExp = NULL;

        delete this;

        return res;

    } else if (typeid(*cond) == typeid(FalseExp)) {
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
Exp IfExp::opt_const_prop(opt_varexp_map &vs, opt_varexp_map &ends) {
    
    cond = cond
            // We will propagate into the condition.
            ->opt_const_prop(vs, ends)
            // Then, we optimize the condition.
            ->optimize();
    
    if (typeid(*cond) == typeid(TrueExp)) {
        // We now know that the expression is always true.
        Exp e = tExp;
        tExp = NULL;

        delete this;
        
        // Hence, we will use that branch to proceed.
        return e->opt_const_prop(vs, ends);
    } else if (typeid(*cond) == typeid(FalseExp)) {
        // We now know that the expression is always true.
        Exp e = fExp;
        fExp = NULL;

        delete this;
        
        // Hence, we will use that branch to proceed.
        return e->opt_const_prop(vs, ends);
    }
    
    // Otherwise, we will only allow propagation to continue for vars
    // that are not affected by either body.
    for (auto a : vs) {
        if (tExp->opt_var_usage(a.first) & 1
        ||  fExp->opt_var_usage(a.first) & 1) {
            delete vs[a.first];
            vs.erase(a.first);
        }
    }
    
    // Now, we proceed with modifications.
    tExp = opt_const_prop(vs, ends);
    fExp = opt_const_prop(vs, ends);

    return this;
}
int IfExp::opt_var_usage(string x) {
    return
        cond->opt_var_usage(x)
      | tExp->opt_var_usage(x)
      | fExp->opt_var_usage(x);
}

int LambdaExp::opt_var_usage(string x) {
    for (int i = 0; xs[i] != ""; i++)
        if (xs[i] == x) return 0;
    
    return exp->opt_var_usage(x);
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
    
    // Now, we must make the changes to the syntax tree depending on whether
    // or not any variables are known to be constants.
    if (consts.size() > 0) {
        body = body->opt_const_prop(consts, ends);

        for (auto x : consts)
            delete x.second;
        
        // Replace any variables with the new versions.
        for (i = 0; exps[i]; i++)
            if (ends.count(ids[i])) {
                delete exps[i];
                exps[i] = ends[ids[i]];
            }
    }

    // Then, we do plain optimization.
    body = body->optimize();

    int nvars = i;
    int kept = 0;
    bool *keep = new bool[nvars];
    for (i = 0; i < nvars; i++) {
        // We only keep variables that will be used for now.
        // In the future, we may wish to handle cases where the current value of x
        // is changed, but not used.
        int use = 0;
        for (int j = 0; j < nvars && !use; j++)
            if (i != j) use = exps[j]->opt_var_usage(ids[i]);
        if (!use) use = body->opt_var_usage(ids[i]);

        if ((keep[i] = use)) {
            kept++;
        } else
            throw_debug("postprocessor", "definition of var '" + ids[i] + "' is not necessary to compute '" + body->toString() + "'");
    }
    
    if (kept == nvars) {
        // Do nothing
        return this;
    } else if (kept) {
        string *xs = new string[kept+1];
        Exp *vs = new Exp[kept+1];
        
        // Build the new expression
        int j;
        for (i = 0, j = 0; i < nvars; i++) {
            if (keep[i]) {
                xs[j] = ids[i];
                vs[j] = exps[i];
                j++;
            } else delete exps[i];
        }
        xs[j] = "";
        vs[j] = NULL;
        
        // Put the new values in place
        delete[] ids; ids = xs;
        delete[] exps; exps = vs;
        delete keep;

        return this;
    } else {
        // None of the variables are necessary, so we can destroy them all.
        Exp e = body;

        body = NULL;
        delete this;
        delete keep;

        return e;
    }
}
int LetExp::opt_var_usage(string x) {
    int use = 0;
    
    for (int i = 0; exps[i] && (use ^ 3); i++) {
        use |= exps[i]->opt_var_usage(x);
        if (ids[i] == x)
            return use;
    }

    return use | body->opt_var_usage(x);
}
Exp LetExp::opt_const_prop(opt_varexp_map& consts, opt_varexp_map &end) {
    // The principle is similar to with the set-exp.

    // Then, we will update the scope.
    for (int i = 0; exps[i]; i++) {
        exps[i] = exps[i]
                ->opt_const_prop(consts, end)
                ->optimize();
        
        // If the expression is constant, propagate it.
        if (is_const_exp(exps[i])) {
            if (consts.count(ids[i]))
                delete consts[ids[i]];
            consts[ids[i]] = exps[i]->clone();
        }
    }

    // Perform propagation.
    body = body->opt_const_prop(consts, end);
    
    return this;
}

Exp ListExp::optimize() {
    auto tmp = new LinkedList<Exp>;
    
    // Optimize each element one by one.
    while (!list->isEmpty())
        tmp->add(0, list->remove(0)->optimize());
    
    // Reassemble the list.
    while (!tmp->isEmpty())
        list->add(0, tmp->remove(0));

    return this;
}
Exp ListExp::opt_const_prop(opt_varexp_map& vs, opt_varexp_map &end) {
    auto tmp = new LinkedList<Exp>;
    
    // Optimize each element one by one.
    while (!list->isEmpty())
        tmp->add(0, list->remove(0)->opt_const_prop(vs, end));
    
    // Reassemble the list.
    while (!tmp->isEmpty())
        list->add(0, tmp->remove(0));

    return this;
}
int ListExp::opt_var_usage(string x) {
    int use = 0;
    auto it = list->iterator();
    
    while (it->hasNext() && use ^ 3)
        use |= it->next()->opt_var_usage(x);
    
    return use;
}

Exp ListAccessExp::optimize() {
    list = list->optimize();
    idx = idx->optimize();

    if (typeid(*list) == typeid(ListExp) && typeid(*idx) == typeid(IntExp)) {
        int i = ((IntExp*) idx)->get();
        auto lst = ((ListExp*) list)->getList();
        if (i >= 0 && i < lst->size()) {
            Exp e = lst->remove(i);
            delete this;
            return e;
        } else {
            return this;
        }
    }
}

Exp MapExp::opt_const_prop(opt_varexp_map &vs, opt_varexp_map &ends) {
    list = list->opt_const_prop(vs, ends);
    
    if (typeid(*func) == typeid(ListExp) && ((ListExp*) list)->getList()->size() == 0) {
        // map [] into f = [] forall f
        Exp e = list; list = NULL;
        delete this;
        return e;
    } if (typeid(*func) == typeid(LambdaExp)) {
        // We can see if the function has side effects
        for (auto x : vs) {
            if (func->opt_var_usage(x.first) & 1) {
                delete vs[x.first];
                vs.erase(x.first);
            }
        }
        
        // Now that any variables chosen will not be affected, we can
        // simply proceed with propagation.
        func = func->opt_const_prop(vs, ends);

        return this;
    } else {
        // No assumptions can be made about the usage of the variables.
        // So, we must drop all of them.
        for (auto x : vs) {
            delete vs[x.first];
            vs.erase(x.first);
        }

        return this;
    }
}

Exp NotExp::optimize() {
    exp = exp->optimize();

    if (typeid(*exp) == typeid(TrueExp)) {
        // not true = false
        delete exp;
        return new FalseExp;
    } else if (typeid(*exp) == typeid(FalseExp)) {
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
        Val v = valueOf(NULL);
        
        if (!v)
            return this;
        else {
            Exp res = reexpress(v);
            delete this;
            return res;
        }
    } else
        return this;
}

Exp OperatorExp::opt_const_prop(opt_varexp_map &vs, opt_varexp_map &end) {
    // The operator expression will simply propagate the constant to its
    // child expressions.
    left = left->opt_const_prop(vs, end);
    right = right->opt_const_prop(vs, end);

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
Exp PrintExp::opt_const_prop(opt_varexp_map& vs, opt_varexp_map& end) {
    int i;
    for (i = 0; args[i]; i++)
        args[i] = args[i]->opt_const_prop(vs, end);

    return this;
}
int PrintExp::opt_var_usage(string x) {
    int res = 0;
    for (int i = 0; res ^ 3 && args[i]; i++)
        res |= args[i]->opt_var_usage(x);

    return res;
}

Exp SetExp::optimize() {
    tgt->optimize();
    exp->optimize();
    return this;
}
Exp SetExp::opt_const_prop(opt_varexp_map &vs, opt_varexp_map &end) {
    // Propagate the value
    exp = exp->opt_const_prop(vs, end);

    if (
    typeid(*tgt) == typeid(VarExp) &&
    vs.count(tgt->toString())
    ) {
        // One of the values is being reassigned. So, we will change
        // the value of the constant in the table or remove it
        // altogether.
        exp = exp->optimize(); // Optimize right now.
        
        // We will save the value as the final outcome
        delete end[tgt->toString()];
        end[tgt->toString()] = exp->clone();

        if (is_const_exp(exp)) {
            // Replace (the var is still constant)
            vs[tgt->toString()] = exp;

            // Plus, we can drop the change in value, since it will be accounted for.
            Exp e = exp->clone();
            exp = NULL;

            delete this;

            return e;
        } else {
            // Delete (the var is no longer constant)
            vs.erase(tgt->toString());
            return this;
        }
    } else return this;
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
Exp SequenceExp::opt_const_prop(opt_varexp_map &vs, opt_varexp_map &end) {
    auto exps = new LinkedList<Exp>;

    while (!seq->isEmpty()) {
        Exp exp = seq->remove(0);
        exp = exp->opt_const_prop(vs, end);
        
        // Keep necessary statements
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
int SequenceExp::opt_var_usage(std::string x) {
    int use = 0;
    auto it = seq->iterator();
    while (it->hasNext() && use ^ 3) {
        use |= it->next()->opt_var_usage(x);
    }
    return use;
}

Exp StdlibOpExp::optimize() {
    x = x->optimize();
    if (is_const_exp(x)) {
        Val v = valueOf(NULL);
        Exp e = reexpress(v);
        delete v;
        delete this;
        return e;
    } else
        return this;
}

Exp VarExp::opt_const_prop(opt_varexp_map &vs, opt_varexp_map &end) {
    if (vs.count(id)) {
        // Because the value is known, we can simply return the
        // value of the variable.
        Exp e = vs[id]->clone();
        delete this;
        return e;
    } else return this;
}

Exp WhileExp::optimize() {
    cond = cond->optimize();

    if (typeid(*cond) == typeid(FalseExp)) {
        // The condition is always false, hence it will never execute.
        delete this;
        return new VoidExp;
    } else {
        body = body->optimize();
        return this;
    }
}
Exp WhileExp::opt_const_prop(opt_varexp_map& vs, opt_varexp_map& end) {
    // We will stop propagating any variables that will change as a result of the loop.
    for (auto x : vs) {
        if (opt_var_usage(x.first) & 1)
            vs.erase(x.first);
    }

    cond->opt_const_prop(vs, end);
    body->opt_const_prop(vs, end);
    return this;
}

