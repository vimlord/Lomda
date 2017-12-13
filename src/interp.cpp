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
        delete exp, env;
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
    Value **xs = (Value**) malloc((argc+1) * sizeof(Value*));
    for (int i = 0; i < argc; i++) {
        // Evanuate the argument
        xs[i] = args[i]->valueOf(env);

        // Success test (every argument must successfully parse
        if (xs[i] == NULL) {
            // An argument failed to parse, so clean up
            return NULL;
        }
    }
    xs[argc] = NULL;

    // Now, compute the answer under the lambda's environment
    Value *y = F->apply(xs);
    
    // Garbage collection on the array
    free(xs);

    return y;
}

Expression* deriveVal(Value *v, int c = 1) {
    if (typeid(*v) == typeid(ListVal)) {
        auto it = ((ListVal*) v)->get()->iterator();
        LinkedList<Expression*> *lst = new LinkedList<Expression*>;
        while (it->hasNext()) {
            Expression *x = deriveVal(it->next());
            lst->add(lst->size(), x);
        }
        return new ListExp(lst); 
    } else return new IntExp(c);
}
Value* DerivativeExp::valueOf(Environment* env) {

    // Only differentiable functions are differentiated
    if (dynamic_cast<const Differentiable*>(func) != nullptr) {
        DerivEnv denv = new LinkedList<DenvFrame>;
        // Perform initial loading
        LinkedList<ExtendEnv*> env_frames;
        Environment *tmp = env;
        while (typeid(*tmp) == typeid(ExtendEnv)) {
            env_frames.add(0, (ExtendEnv*) tmp);
            tmp = tmp->subenvironment();
        }
        
        DenvFrame f;
        f.id = var; f.val = new IntExp(1);
        denv->add(0, f);

        while (!env_frames.isEmpty()) {
            ExtendEnv *ee = env_frames.remove(0);
            Value *v = ee->topVal();
            
            f.id = ee->topId();
            
            if (typeid(*v) == typeid(LambdaVal)) {
                // Lambda derivative: d/dx lambda (x) f(x) = lambda (x) d/dx f(x)
                // We will need the derivative for this

                // Get the function and its components
                LambdaVal *func = (LambdaVal*) v;
                string *ids = func->getArgs();
                Expression *body = func->getBody();
                if (dynamic_cast<const Differentiable*>(body) == nullptr) {
                    // Using a non-differentiable function is a horrible idea
                    delete denv;
                    return NULL;
                }
                LambdaExp *exp = new LambdaExp(ids, body);
                f.val = exp->derivativeOf(denv);
            } else {
                // Trivial derivative: d/dx c = 0
                f.val = deriveVal(v, f.id == var ? 1 : 0);
            }

            denv->add(0, f);
        }

        // Get an expression that represents the result
        Expression *e = ((Differentiable*) func)->derivativeOf(denv);
        
        // Perform garbage collection on the derivative environment
        while (!denv->isEmpty()) delete denv->remove(0).val;
        delete denv;
        
        // And then compute it if possible
        if (e) {
            Value *v = e->valueOf(env);

            delete e; // Garbage collection

            return v;
        } else
            return NULL;
    } else {
        
        return NULL;
    }
}

Value* FalseExp::valueOf(Environment* env) { return new BoolVal(false); }

Value* ForExp::valueOf(Environment *env) {
    // Evaluate the list
    Value *listExp = set->valueOf(env);
    if (!listExp || typeid(*listExp) != typeid(ListVal))
        return NULL;
    List<Value*> *list = ((ListVal*) listExp)->get();
    
    // Gather an iterator
    Iterator<int, Value*> *it = list->iterator();

    // Value to be return
    Value *v = new VoidVal;

    while (it->hasNext()) {
        // Get the next item from the list
        Value *x = it->next();
        
        // Build an environment
        Environment *e = new ExtendEnv(id, x, env);
        
        v = body->valueOf(e);

        if (!v) {
            delete it;
            return NULL;
        }

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

    if (bRes)
        return tExp->valueOf(env);
    else
        return fExp->valueOf(env);
}

Value* IntExp::valueOf(Environment *env) {
    return new IntVal(val);
}

Value* LambdaExp::valueOf(Environment *env) {
    return new LambdaVal(xs, exp, env);
}

Value* LetExp::valueOf(Environment* env) {
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
            while (i--) env = env->subenvironment();
            return NULL;
        }
        
        // Add it to the environment
        v = v->copy();
        env = new ExtendEnv(ids[i], v, env);

        // We permit all lambdas to have recursive behavior
        if (typeid(*v) == typeid(LambdaVal)) {
            lambdas.add(0, (LambdaVal*) v);
        }
    }
    
    // Apply recursive principles
    while (!lambdas.isEmpty()) {
        LambdaVal *v = lambdas.remove(0);
        v->setEnv(env);
    }

    // Compute the result
    Value *y = body->valueOf(env);
    
    // Garbage collection
    // Not implemented yet
        
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
            delete it;
            return NULL;
        } else
            val->get()->add(i, v);
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
        return NULL;
    }
    
    // The list
    List<Value*> *vals = ((ListVal*) lst)->get();
    
    // Get the index
    Value *f = from->valueOf(env);
    if (!f) return NULL;
    else if (typeid(*f) != typeid(IntVal)) {
        throw_type_err(from, "integer");
        return NULL;
    }
    int i = ((IntVal*) f)->get();

    // Get the index
    Value *t = to->valueOf(env);
    if (!t) return NULL;
    else if (typeid(*t) != typeid(IntVal)) {
        throw_type_err(to, "integer");
        return NULL;
    }
    int j = ((IntVal*) t)->get();

    // Get the item
    LinkedList<Value*> *vs = new LinkedList<Value*>;
    
    auto it = vals->iterator();
    int x;
    for (x = 0; x < i; x++) it->next();
    for (;x < j && it->hasNext(); x++) vs->add(x-i, it->next());

    return new ListVal(vs);

}

Value* NotExp::valueOf(Environment* env) {
    Value *v = exp->valueOf(env);
    
    if (!v)
        return NULL;
    else if (typeid(*v) != typeid(BoolVal)) {
        throw_type_err(exp, "boolean");
        return NULL;
    } else {
        BoolVal *B = (BoolVal*) v;
        bool b = B->get();

        return new BoolVal(!b);
    }
}

Value* OperatorExp::valueOf(Environment *env) {
    Value *a = left->valueOf(env);
    Value *b = right->valueOf(env);

    if (!a || !b) {
        delete a, b;
        return NULL;
    } else
        return op(a, b);
}

Value* RealExp::valueOf(Environment *env) {
    return new RealVal(val);
}

Value* SequenceExp::valueOf(Environment *env) {
    Value *v = pre->valueOf(env);

    if (!v) return NULL;
    else if (!post) return v;
    else return post->valueOf(env);
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

