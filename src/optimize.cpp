#include "expression.hpp"

inline bool is_const_exp(Exp exp) {
    return
        typeid(*exp) == typeid(FalseExp) ||
        typeid(*exp) == typeid(IntExp) ||
        typeid(*exp) == typeid(RealExp) ||
        typeid(*exp) == typeid(StringExp) ||
        typeid(*exp) == typeid(TrueExp);
}

Exp IfExp::optimize() {
    Exp res = this;

    cond = cond->optimize();
    if (typeid(*cond) == typeid(TrueExp)) {
        // Because the condition is always true, the if statement
        // is redundant. So, we drop all but the true expression.
        delete cond;
        delete fExp;

        res = tExp->optimize();
        tExp = NULL;

        delete this;
    } else if (typeid(*cond) == typeid(FalseExp)) {
        // Similarly, because the expression is always false, we
        // will simply drop the true body and the condition.
        delete cond;
        delete tExp;

        res = fExp->optimize();
        fExp = NULL;

        delete this;
    } else {
        tExp = tExp->optimize();
        fExp = fExp->optimize();
    }

    return res;
}

Exp LetExp::optimize() {
    // For a let-exp, we seek to propagate constant variables through the
    // expression and replace them wherever possible. Then, we can allow
    // constant folding to further reduce.

    std::unordered_map<std::string, Exp> consts;

    for (int i = 0; exps[i]; i++) {
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
            consts.erase(x.first);
    }

    // Then, we do plain optimization.
    body = body->optimize();

    // TODO: Remove unused variables
    
    return this;
}

Exp LetExp::opt_const_prop(std::unordered_map<std::string, Exp>& consts) {
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

Exp OperatorExp::opt_const_prop(std::unordered_map<std::string, Exp> &vs) {
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
Exp SetExp::opt_const_prop(std::unordered_map<std::string, Exp> &vs) {
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
Exp SequenceExp::opt_const_prop(std::unordered_map<std::string, Exp> &vs) {
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

Exp VarExp::opt_const_prop(std::unordered_map<std::string, Exp> &vs) {
    if (vs.count(id)) {
        // Because the value is known, we can simply return the
        // value of the variable.
        Exp e = vs[id]->clone();
        delete this;
        return e;
    } else return this;
 }

