#include "interp.hpp"

#include "expression.hpp"
#include "value.hpp"
#include "environment.hpp"

#include "bnf.hpp"

#include <cstring>

using namespace std;

Value* run(string program) {
    Expression *exp = compile(program);
    if (exp) {
        // A tree was successfully parsed, so run it.
        Environment *env = new EmptyEnv();
        Value *val = exp->valueOf(env);
        delete exp;
        delete env;
        //if (!val) throw_err("runtime", "could not evaluate expression");
        return val;
    } else {
        return NULL;
    }
}

Value* ApplyExp::valueOf(Environment *env) {
    Value *f = op->valueOf(env);

    // Null check the function
    if (!f)
        return NULL;
    
    // Type check the function
    if (typeid(*f) != typeid(LambdaVal)) {
        throw_type_err(op, "lambda");
        return NULL;
    }
    LambdaVal *F = (LambdaVal*) f;

    int argc = 0;
    while (args[argc]) argc++;

    // Operate on each argument under the given environment
    Value **xs = new Value*[argc+1];
    int i;
    for (i = 0; i < argc; i++) {
        // Evanuate the argument
        xs[i] = args[i]->valueOf(env);

        // Success test (every argument must successfully parse
        if (xs[i] == NULL) {
            // Garbage collect the list
            while (i--) xs[i]->rem_ref();
            delete xs;

            // An argument failed to parse, so clean up
            return NULL;
        }
    }
    xs[argc] = NULL;

    // Now, compute the answer under the lambda's environment
    Value *y = F->apply(xs);
    
    // Garbage collection on the array
    while (i--) xs[i]->rem_ref();
    delete xs;

    return y;
}

Value* deriveVal(Value *v, int c = 1) {
    if (typeid(*v) == typeid(ListVal)) {
        auto it = ((ListVal*) v)->get()->iterator();
        LinkedList<Value*> *lst = new LinkedList<Value*>;
        while (it->hasNext()) {
            Value *x = deriveVal(it->next());
            lst->add(lst->size(), x);
        }
        return new ListVal(lst); 
    } else return new IntVal(c);
}
Value* DerivativeExp::valueOf(Environment* env) {

    // Only differentiable functions are differentiated
    if (dynamic_cast<const Differentiable*>(func) != nullptr) {
        // Base case: we know of nothing
        Environment *denv = new EmptyEnv;

        // Perform initial loading
        LinkedList<ExtendEnv*> env_frames;
        Environment *tmp = env;
        while (typeid(*tmp) == typeid(ExtendEnv)) {
            env_frames.add(0, (ExtendEnv*) tmp);
            tmp = tmp->subenvironment();
        }
        
        while (!env_frames.isEmpty()) {
            ExtendEnv *ee = env_frames.remove(0);
            Value *v = ee->topVal();
            string id = ee->topId();
            
            if (typeid(*v) == typeid(LambdaVal)) {
                LambdaVal *lv = (LambdaVal*) v;
                int i;
                for (i = 0; lv->getArgs()[i] != ""; i++);

                string *ids = new string[i+1];
                ids[i] = "";
                while (i--) ids[i] = lv->getArgs()[i];

                LambdaVal *dv = new LambdaVal(ids, 
                                new DerivativeExp(
                                        lv->getBody()->clone(), var
                                ), lv->getEnv()->clone());

                // Lambda derivative: d/dx lambda (x) f(x) = lambda (x) d/dx f(x)
                // We will need the derivative for this
                denv = new ExtendEnv(id, dv, denv);
                dv->rem_ref(); // The derivative exists only within the environment
            } else {
                // Trivial derivative: d/dx c = 0
                Value *c = deriveVal(v, id == var ? 1 : 0);
                denv = new ExtendEnv(id, c, denv);
                c->rem_ref(); // The derivative exists only within the environment
            }
        }

        // Now, we have the variable, the environment, and the differentiable
        // environment. So, we can simply derive and return the result.
        Value *res = ((Differentiable*) func)->derivativeOf(var, env, denv);

        // Garbage collection
        delete denv;

        return res;

    } else {
        throw_err("runtime", "expression '" + func->toString() + "' is non-differentiable");
        return NULL;
    }
}

Value* FalseExp::valueOf(Environment* env) { return new BoolVal(false); }

Value* ForExp::valueOf(Environment *env) {
    // Evaluate the list
    Value *listExp = set->valueOf(env);
    if (!listExp) return NULL;
    else if (typeid(*listExp) != typeid(ListVal)) {
        throw_type_err(set, "list");
        return NULL;
    }
    List<Value*> *list = ((ListVal*) listExp)->get();
    
    // Gather an iterator
    Iterator<int, Value*> *it = list->iterator();

    // Value to be return
    Value *v = new VoidVal;

    while (it->hasNext()) {
        // Get the next item from the list
        Value *x = it->next();
        
        // Build an environment
        Environment *e = new ExtendEnv(id, x, env->clone());

        v->rem_ref();
        v = body->valueOf(e);

    }

    delete it;

    return v;

}

Value* IfExp::valueOf(Environment *env) {
    Value *b = cond->valueOf(env);

    if (typeid(*b) != typeid(BoolVal)) {
        return NULL;
    }

    BoolVal *B = (BoolVal*) b;
    bool bRes = B->get();

    // Conditional garbage collection
    b->rem_ref();

    if (bRes)
        return tExp->valueOf(env);
    else
        return fExp->valueOf(env);
}

Value* IntExp::valueOf(Environment *env) {
    return new IntVal(val);
}

Value* LambdaExp::valueOf(Environment *env) {
    int argc;
    for (argc = 0; xs[argc] != ""; argc++);

    string *ids = new string[argc+1];
    ids[argc] = "";
    while (argc--) ids[argc] = xs[argc];

    return new LambdaVal(ids, exp->clone(), env->clone());
}

Value* LetExp::valueOf(Environment* env) {
    // We will operate on a clone
    env = env->clone();

    int argc = 0;
    for (; exps[argc]; argc++);

    // I want to make all of the lambdas recursive.
    // So, I will track my lambdas for now
    LinkedList<LambdaVal*> lambdas;
    
    // Extend the environment
    for (int i = 0; i < argc; i++) {
        // Compute the expression
        Value *v = exps[i]->valueOf(env);
        if (!v) {
            // Garbage collection will happen here
            delete env;
            return NULL;
        }
        
        // Add it to the environment
        Value *x = v->clone();
        env = new ExtendEnv(ids[i], x, env);
        
        // Drop references
        v->rem_ref();
        x->rem_ref();

        // We permit all lambdas to have recursive behavior
        if (typeid(*x) == typeid(LambdaVal)) {
            lambdas.add(0, (LambdaVal*) x);
        }

    }
    
    // Apply recursive principles
    while (!lambdas.isEmpty()) {
        LambdaVal *v = lambdas.remove(0);
        v->setEnv(env->clone());

        // Although the environment contains itself, a self-reference
        // doesn't really count. In fact, it's a bitch to deal with.
        v->rem_ref();
    }

    // Compute the result
    Value *y = body->valueOf(env);

    // Garbage collection
    delete env;
        
    // Return the result
    return y;

}

Value* ListExp::valueOf(Environment *env) {
    // Generate a blank list
    ListVal *val = new ListVal();
    
    // Add each item
    Iterator<int, Expression*> *it = list->iterator();
    for(int i = 0; it->hasNext(); i++) {
        // Compute the value of each item
        Value *v = it->next()->valueOf(env);
        if (!v) {
            // Garbage collection on the iterator and the value
            delete it;
            delete val;

            return NULL;
        } else {
            val->get()->add(i, v);
        }
    }

    delete it;
    return val;
}

Value* ListAccessExp::valueOf(Environment *env) {
    // Get the list
    Value *f = list->valueOf(env);
    if (!f)
        return NULL;
    else if (typeid(*f) != typeid(ListVal)) {
        throw_type_err(list, "list");
        return NULL;
    }
    
    // The list
    List<Value*> *vals = ((ListVal*) f)->get();
    
    // Get the index
    Value *index = idx->valueOf(env);
    if (!index) return NULL;
    else if (typeid(*index) != typeid(IntVal)) {
        throw_type_err(idx, "integer");
        return NULL;
    }
    int i = ((IntVal*) index)->get();

    // Get the item
    return vals->get(i);

}

Value* ListAddExp::valueOf(Environment *env) {
    // Get the list
    Value *f = list->valueOf(env);
    if (!f)
        return NULL;
    else if (typeid(*f) != typeid(ListVal)) {
        throw_type_err(list, "list");
        return NULL;
    }
    List<Value*> *vals = ((ListVal*) f)->get();

    // Compute the index
    Value *index = idx->valueOf(env);
    if (!index) return NULL;
    else if (typeid(*index) != typeid(IntVal)) {
        throw_type_err(idx, "integer");
        return NULL;
    }
    int i = ((IntVal*) index)->get();

    // Compute the value
    Value *val = elem->valueOf(env);
    if (!index)
        return NULL;
    
    // Remove and return the item
    vals->add(i, val);

    return new VoidVal;
}

Value* ListRemExp::valueOf(Environment *env) {
    // Get the list
    Value *f = list->valueOf(env);
    if (!f)
        return NULL;
    else if (typeid(*f) != typeid(ListVal)) {
        throw_type_err(list, "list");
        return NULL;
    }
    List<Value*> *vals = ((ListVal*) f)->get();

    // Compute the index
    Value *index = idx->valueOf(env);
    if (!index) return NULL;
    else if (typeid(*index) != typeid(IntVal)) {
        throw_type_err(idx, "integer");
        return NULL;
    }
    int i = ((IntVal*) index)->get();
    
    // Remove and return the item
    return vals->remove(i);
}

Value* ListSliceExp::valueOf(Environment *env) {
    // Get the list
    Value *lst = list->valueOf(env);
    if (!lst)
        return NULL;
    else if (typeid(*lst) != typeid(ListVal)) {
        throw_type_err(list, "list");
        lst->rem_ref(); // Garbage collection
        return NULL;
    }
    
    // The list
    List<Value*> *vals = ((ListVal*) lst)->get();

    // Get the index
    Value *f = from->valueOf(env);
    if (!f) return NULL;
    else if (typeid(*f) != typeid(IntVal)) {
        throw_type_err(from, "integer");
        lst->rem_ref(); // Garbage collection
        return NULL;
    }
    int i = ((IntVal*) f)->get();

    // Get the index
    Value *t = to->valueOf(env);
    if (!t) return NULL;
    else if (typeid(*t) != typeid(IntVal)) {
        throw_type_err(to, "integer");
        lst->rem_ref(); // Garbage collection
        return NULL;
    }
    int j = ((IntVal*) t)->get();

    // Get the item
    LinkedList<Value*> *vs = new LinkedList<Value*>;
    
    auto it = vals->iterator();
    int x;
    for (x = 0; x < i; x++) it->next();
    for (;x < j && it->hasNext(); x++) {
        // Add the value and a reference to it
        Value *v = it->next();
        vs->add(x-i, v);
        v->add_ref();
    }
    
    // Garbage collection
    lst->rem_ref();
    delete it;

    return new ListVal(vs);

}

Value* MagnitudeExp::valueOf(Environment *env) {
    Value *v = exp->valueOf(env);
    Value *res = NULL;

    if (!v) return NULL;
    else if (typeid(*v) == typeid(IntVal)) {
        // Magnitude of number is its absolute value
        int val = ((IntVal*) v)->get();
        res = new IntVal(val > 0 ? val : -val);
    } else if (typeid(*v) == typeid(RealVal)) {
        // Magnitude of number is its absolute value
        int val = ((RealVal*) v)->get();
        res = new RealVal(val > 0 ? val : -val);
    } else if (typeid(*v) == typeid(ListVal)) {
        // Magnitude of list is its length
        int val = ((ListVal*) v)->get()->size();
        res = new IntVal(val > 0 ? val : -val);
    }
    
    // Garbage collection
    v->rem_ref();

    return res;
}

Value* NotExp::valueOf(Environment* env) {
    Value *v = exp->valueOf(env);
    
    if (!v)
        return NULL;
    else if (typeid(*v) != typeid(BoolVal)) {
        throw_type_err(exp, "boolean");
        v->rem_ref(); // Garbage
        return NULL;
    } else {
        BoolVal *B = (BoolVal*) v;
        bool b = B->get();
        
        // Garbage
        v->rem_ref();

        return new BoolVal(!b);
    }
}

Value* OperatorExp::valueOf(Environment *env) {
    Value *a = left->valueOf(env);
    if (!a) return NULL;

    Value *b = right->valueOf(env);

    if (!b) {
        a->rem_ref();
        return NULL;
    } else {
        Value *res = op(a, b);
        a->rem_ref();
        b->rem_ref();
        return res;
    }
}

Value* RealExp::valueOf(Environment *env) {
    return new RealVal(val);
}

Value* SequenceExp::valueOf(Environment *env) {
    Value *v = pre->valueOf(env);

    if (!v) return NULL;
    else if (!post) return v;
    else {
        v->rem_ref();
        return post->valueOf(env);
    }
}

Value* SetExp::valueOf(Environment *env) {
    Value *v = NULL;

    for (int i = 0; exps[i]; i++) {
        // Get info for modifying the environment
        Value *u = tgts[i]->valueOf(env);
        if (!u)
            // The variable doesn't exist
            return NULL;

        // Evaluate the expression
        v = exps[i]->valueOf(env);

        if (!v)
            // The value could not be evaluated.
            return NULL;
        
        // Set the new value
        if (u->set(v))
            return NULL;

    }
    
    // To be simple, we return 0 on completion
    // as opposed to NULL, which indicates a failure.
    return v ? v : new VoidVal;
}

Value* TrueExp::valueOf(Environment* env) { return new BoolVal(true); }

Value* VarExp::valueOf(Environment *env) {
    Value *res = env->apply(id);
    if (!res) throw_err("runtime", "variable '" + id + "' was not recognized");
    else res->add_ref(); // This necessarily creates a new reference. So, we must track it.
    

    return res;
}

Value* WhileExp::valueOf(Environment *env) {
    bool skip = alwaysEnter;
    Value *v = new VoidVal;

    while (true) {
        Value *c = cond->valueOf(env);
        if (!c) return NULL;
        else if (typeid(*c) != typeid(BoolVal)) {
            throw_type_err(cond, "boolean");
            return NULL;
        } else if (skip || ((BoolVal*) c)->get()) {
            // If do-while was enacted, cease the action.
            skip = false;
            // Compute the new outcome. If it is
            // NULL, computation failed, so NULL
            // should be returned.
            if (!(v = body->valueOf(env)))
                return NULL;
        } else
            return v;
    }
}

