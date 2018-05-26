#include "expression.hpp"

bool AdtExp::postprocessor(Trie<bool> *vars) { return true; }
bool AdtDeclarationExp::postprocessor(Trie<bool> *vars) {
    // We forbid repetition of kind names.
    for (int i = 0; argss[i]; i++)
        for (int j = i+1; argss[j]; j++)
            if (ids[i] == ids[j]) {
                throw_err("", "repetition of adt case " + ids[i] + " is not permitted");
                return NULL;
            }
    
    // We also forbid redefinitions of variables.
    if (vars->hasKey(name)) {
        throw_err("", "definition of ADT " + name + " conflicts with variable using that name");
        return false;
    } else
        vars->add(name, true);

    bool res = body->postprocessor(vars);

    vars->remove(name);

    return res;
}
bool SwitchExp::postprocessor(Trie<bool> *vars) {
    // Process the ADT
    if (!adt->postprocessor(vars)) return false;
    
    // Process each of the branches
    for (int i = 0; bodies[i]; i++) {
        // Verify that there are no repeats
        for (int j = i+1; bodies[j]; j++)
            if (names[i] == names[j]) {
                throw_err("", "repetition of adt case " + names[i] + " is not permitted");
                return false;
            }

        // Verify that the arguments are unclaimed.
        for (int j = 0; idss[i][j] != ""; j++)
            if (vars->hasKey(idss[i][j])) {
                throw_err("", "redefinition of variable " + idss[i][j] + " is not permitted");
                while (j--) vars->remove(idss[i][j]);
                return false;
            } else
                vars->add(idss[i][j], true);
        
        // In this scope, we can check the body.
        auto res = bodies[i]->postprocessor(vars);
        
        // Reset the variable set
        for (int j = 0; idss[i][j] != ""; j++)
            vars->remove(idss[i][j]);
        
        if (!res) return false;
    }

    return true;
}

bool ApplyExp::postprocessor(Trie<bool> *vars) {
    if (!op->postprocessor(vars)) return false;

    auto res = true;
    for (int i = 0; res && args[i]; i++)
        res = args[i]->postprocessor(vars);

    return res;
}

bool FoldExp::postprocessor(Trie<bool> *vars) {
    return list->postprocessor(vars)
        && func->postprocessor(vars)
        && base->postprocessor(vars);}

bool LambdaExp::postprocessor(Trie<bool> *vars) {

    auto trie = new Trie<bool>;
    for (int i = 0; xs[i] != ""; i++)
        trie->add(xs[i], true);
    
    auto it = vars->iterator();

    while (it->hasNext()) {
        auto k = it->next();
        if (!trie->hasKey(k))
            trie->add(k, true);
    }
    delete it;
    
    bool res = exp->postprocessor(trie);

    delete trie;
    return res;
}

bool LetExp::postprocessor(Trie<bool> *vars) {
    bool res = true;

    // Evaluate the processing of the non-recursive variables
    for (int i = 0; res && exps[i]; i++)
        if (!rec[i] && !exps[i]->postprocessor(vars)) {
            std::cout << "in definition " + ids[i] + " = " + exps[i]->toString() << "\n";
            std::cout << "in expression '" + toString() + "'\n";
            return NULL;
        }
    
    // Add the variables to the recognized varset
    for (int i = 0; res && ids[i] != ""; i++)
        if (vars->hasKey(ids[i])) {
            // Error message
            throw_err("", "redefinition of variable " + ids[i] + " is not permitted");
            std::cout << "in expression '" + toString() + "'\n";

            // Cleanup
            while (i--) vars->remove(ids[i]);
            return NULL;
        } else {
            vars->add(ids[i], true);
        }
    
    // Evaluate the processing of the recursive variables
    for (int i = 0; res && exps[i]; i++)
        if (rec[i] && !exps[i]->postprocessor(vars)) {
            std::cout << "in recursive definition " + ids[i] + " = " + exps[i]->toString() << "\n";
            res = false;
        }
    
    if (res) {
        res = body->postprocessor(vars);
        if (!res && typeid(*body) != typeid(LetExp))
            std::cout << "in expression '" + body->toString() + "'\n";
    }

    for (int i = 0; ids[i] != ""; i++)
        vars->remove(ids[i]);
    
    if (!res)
        std::cout << "in expression '" + toString() + "'\n";

    return res;
}

bool SequenceExp::postprocessor(Trie<bool> *vars) {
    auto it = seq->iterator();
    auto res = true;
    while (res && it->hasNext())
        res = it->next()->postprocessor(vars);
    delete it;

    return res;
}

bool ForExp::postprocessor(Trie<bool> *vars) {
    if(!set->postprocessor(vars)) return false;
    if (vars->hasKey(id)) {
        throw_err("", "redefinition of variable " + id + " is not permitted");
        return false;
    }
    vars->add(id, true);
    bool res = body->postprocessor(vars); 
    vars->remove(id);

    return res;
}
bool HasExp::postprocessor(Trie<bool> *vars) { return item->postprocessor(vars) && set->postprocessor(vars); }
bool IsaExp::postprocessor(Trie<bool> *vars) { return exp->postprocessor(vars); }
bool MagnitudeExp::postprocessor(Trie<bool> *vars) { return exp->postprocessor(vars); }
bool NormExp::postprocessor(Trie<bool> *vars) { return exp->postprocessor(vars); }
bool NotExp::postprocessor(Trie<bool> *vars) { return exp->postprocessor(vars); }
bool SetExp::postprocessor(Trie<bool> *vars) { return tgt->postprocessor(vars) && exp->postprocessor(vars); }
bool ThunkExp::postprocessor(Trie<bool> *vars) { return exp->postprocessor(vars); }
bool WhileExp::postprocessor(Trie<bool> *vars) { return cond->postprocessor(vars) && body->postprocessor(vars); }
