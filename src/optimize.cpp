#include "expression.hpp"

using namespace std;

inline bool is_const_exp(Exp exp) {
    return
        typeid(*exp) == typeid(FalseExp) ||
        typeid(*exp) == typeid(IntExp) ||
        typeid(*exp) == typeid(RealExp) ||
        typeid(*exp) == typeid(StringExp) ||
        typeid(*exp) == typeid(TrueExp);
}
int ApplyExp::opt_var_usage(string x) {
    int use = op->opt_var_usage(x);

    for (int i = 0; args[i] && (use ^ 3); i++)
        use |= args[i]->opt_var_usage(x);

    return use;
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

int ListExp::opt_var_usage(string x) {
    int use = 0;
    auto it = list->iterator();
    
    while (it->hasNext() && use ^ 3)
        use |= it->next()->opt_var_usage(x);
    
    return use;
}

Exp LetExp::optimize() {
    // For a let-exp, we seek to propagate constant variables through the
    // expression and replace them wherever possible. Then, we can allow
    // constant folding to further reduce.

    unordered_map<string, Exp> consts;
    
    int i;
    for (i = 0; exps[i]; i++) {
        exps[i] = exps[i]->optimize();
        
        // If the optimal value is constant, then we should store
        // it for the purpose of constant propagation.
        if (is_const_exp(exps[i])) {
            throw_debug("postprocessor", "will attempt to propagate constant " + ids[i] + " as " + exps[i]->toString());
            consts[ids[i]] = exps[i]->clone();
        }
    }
    
    // Now, we must make the changes to the syntax tree depending on whether
    // or not any variables are known to be constants.
    if (consts.size() > 0) {
        body = body->opt_const_prop(consts);

        for (auto x : consts)
            delete x.second;
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
        int use = body->opt_var_usage(ids[i]);
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
Exp LetExp::opt_const_prop(unordered_map<string, Exp>& consts) {
    // The principle is similar to with the set-exp.

    // Then, we will update the scope.
    for (int i = 0; exps[i]; i++) {
        exps[i] = exps[i]
                ->opt_const_prop(consts)
                ->optimize();
        
        // If the expression is constant, propagate it.
        if (is_const_exp(exps[i])) {
            if (consts.count(ids[i]))
                delete consts[ids[i]];
            consts[ids[i]] = exps[i]->clone();
        }
    }

    // Perform propagation.
    body = body->opt_const_prop(consts);
    
    return this;
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

Exp OperatorExp::opt_const_prop(unordered_map<string, Exp> &vs) {
    // The operator expression will simply propagate the constant to its
    // child expressions.
    left = left->opt_const_prop(vs);
    right = right->opt_const_prop(vs);

    return this;
}

Exp SetExp::optimize() {
    tgt->optimize();
    exp->optimize();
    return this;
}
Exp SetExp::opt_const_prop(unordered_map<string, Exp> &vs) {
    // Propagate the value
    exp = exp->opt_const_prop(vs);

    if (
    typeid(*tgt) == typeid(VarExp) &&
    vs.count(tgt->toString())
    ) {
        // One of the values is being reassigned. So, we will change
        // the value of the constant in the table or remove it
        // altogether.
        exp = exp->optimize(); // Optimize right now.

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
    }
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
Exp SequenceExp::opt_const_prop(unordered_map<string, Exp> &vs) {
    auto exps = new LinkedList<Exp>;

    while (!seq->isEmpty()) {
        Exp exp = seq->remove(0);
        exp = exp->opt_const_prop(vs);
        
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
}

Exp VarExp::opt_const_prop(unordered_map<string, Exp> &vs) {
    if (vs.count(id)) {
        // Because the value is known, we can simply return the
        // value of the variable.
        Exp e = vs[id]->clone();
        delete this;
        return e;
    } else return this;
}


