#include "interp.hpp"

#include "expression.hpp"
#include "value.hpp"
#include "environment.hpp"

#include "bnf.hpp"
#include "config.hpp"

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
            
            // And the function
            F->rem_ref();

            // An argument failed to parse, so clean up
            return NULL;
        }
    }
    xs[argc] = NULL;

    // Now, compute the answer under the lambda's environment
    Value *y = F->apply(xs);
    
    // Garbage collection on the array
    while (i--) xs[i]->rem_ref();
    delete[] xs;

    // And the function
    F->rem_ref();

    return y;
}

Value* deriveVal(Value *v, int c = 1) {
    if (typeid(*v) == typeid(StringVal) ||
        typeid(*v) == typeid(BoolVal))
        // Certain types are non-differentiable
        return NULL;
    else if (typeid(*v) == typeid(ListVal)) {
        auto it = ((ListVal*) v)->get()->iterator();
        LinkedList<Value*> *lst = new LinkedList<Value*>;
        while (it->hasNext()) {
            Value *x = deriveVal(it->next());
            lst->add(lst->size(), x);
        }
        return new ListVal(lst); 
    } else if (typeid(*v) == typeid(MatrixVal)) {
        Matrix M(((MatrixVal*) v)->get().R, ((MatrixVal*) v)->get().C);
        for (int i = 0; i < M.R; i++)
            for (int j = 0; j < M.C; j++)
                M(i, j) = c;
        return new MatrixVal(M);
    } else
        return new IntVal(c);
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

                if (c) {
                    // The value is of a differentiable type
                    denv = new ExtendEnv(id, c, denv);
                    c->rem_ref(); // The derivative exists only within the environment
                }
            }
        }

        // Now, we have the variable, the environment, and the differentiable
        // environment. So, we can simply derive and return the result.
        //std::cout << "compute d/d" << var << " | env ::= " << *env << ", denv ::= " << *denv << "\n";
        Differentiable *df = (Differentiable*) func;
        Value *res = df->derivativeOf(var, env, denv);

        // Garbage collection
        delete denv;

        return res;

    } else {
        throw_err("runtime", "expression '" + func->toString() + "' is non-differentiable");
        return NULL;
    }
}

Value* FalseExp::valueOf(Environment *env) { return new BoolVal(false); }

Value* FoldExp::valueOf(Environment *env) {
    Value *lst = list->valueOf(env);
    if (!lst) return NULL;
    else if (typeid(*lst) != typeid(ListVal)) {
        lst->rem_ref();
        throw_type_err(list, "list");
        return NULL;
    }
    
    Value *f = func->valueOf(env);
    if (!f) return NULL;
    else if (typeid(*f) != typeid(LambdaVal)) {
        throw_type_err(func, "lambda");
        return NULL;
    }

    LambdaVal *fn = (LambdaVal*) f;
    int i;
    for (i = 0; fn->getArgs()[i] != ""; i++);
    if (i != 2) {
        throw_err("runtime", "function defined by '" + func->toString() + "' does not take exactly two arguments");
        return NULL;
    }

    auto it = ((ListVal*) lst)->get()->iterator();
    Value *xs[3];
    xs[0] = base->valueOf(env); // First slot is the accumulator
    xs[2] = NULL; // Last slot is a null terminator.

    while (it->hasNext() && xs[0]) {
        // Second slot is the element
        xs[1] = it->next();
        
        // Update the accumulator.
        Value *acc = fn->apply(xs);
        xs[0]->rem_ref();
        xs[0] = acc;
    }
    
    delete it;
    return xs[0];
}

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
    ListVal *val = new ListVal;
    
    // Add each item
    auto it = list->iterator();
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
    } else if (typeid(*v) == typeid(MatrixVal)) {
        float val = ((MatrixVal*) v)->get().determinant();
        res = new RealVal(val);
    }
    
    // Garbage collection
    v->rem_ref();

    return res;
}

Value* MapExp::valueOf(Environment *env) {
    Value *vs = list->valueOf(env);
    if (!vs) return NULL;

    Value *f = func->valueOf(env);
    if (!f) { vs->rem_ref(); return NULL; }
    
    if (typeid(*f) != typeid(LambdaVal)) {
        throw_type_err(func, "lambda");
        vs->rem_ref();
        return NULL;
    }

    // Extract the function... require that it have one argument
    LambdaVal *fn = (LambdaVal*) f;
    if (fn->getArgs()[0] == "" || fn->getArgs()[1] != "") {
        throw_err("runtime", "map function '" + fn->toString() + "' does not take exactly one argument");
        fn->rem_ref();
        vs->rem_ref();
        return NULL;
    }

    // Get the arguments
    Expression *map = fn->getBody();

    if (typeid(*vs) == typeid(ListVal)) {
        // Given a list, map each element of the list
        ListVal *vals = (ListVal*) vs;
        ListVal *res = new ListVal();
        
        int i = 0;
        auto it = vals->get()->iterator();
        while (it->hasNext()) {
            Value *v = it->next();

            Value **xs = new Value*[2];
            xs[0] = v;
            xs[1] = NULL;

            Value *elem = fn->apply(xs);

            delete[] xs;

            if (!elem) {
                fn->rem_ref();
                vals->rem_ref();
                res->rem_ref();
                return NULL;
            } else {
                res->get()->add(res->get()->size(), elem);
            }

            i++;
        }
        delete it;
        
        fn->rem_ref();
        vals->rem_ref();

        return res;

    } else if (typeid(*vs) == typeid(MatrixVal)) {
        Matrix v = ((MatrixVal*) vs)->get();
        Matrix M(v.R, v.C);

        Value *xs[2];
        xs[1] = NULL;
        
        for (int r = 0; r < M.R; r++)
        for (int c = 0; c < M.C; c++) {
            xs[0] = new RealVal(v(r, c));

            Value *elem = fn->apply(xs);
            
            // Garbage collection
            xs[0]->rem_ref();

            if (!elem) {
                // The matrix is non-computable, so failure!
                fn->rem_ref();
                vs->rem_ref();
                return NULL;
            } else if (typeid(*elem) == typeid(RealVal)) {
                M(r, c) = ((RealVal*) elem)->get();
            } else if (typeid(*elem) == typeid(IntVal)) {
                M(r, c) = (float) (((IntVal*) elem)->get());
            } else {
                elem->rem_ref();
                fn->rem_ref();
                vs->rem_ref();
                return NULL;
            }
        }

        fn->rem_ref();
        vs->rem_ref();
        return new MatrixVal(M);

    } else if (WERROR()) {
        throw_err("runtime", "expression '" + list->toString() + " does not evaluate as list");
        vs->rem_ref();
        fn->rem_ref();
        return NULL;
    } else {
        throw_warning("runtime", "expression '" + list->toString() + " does not evaluate as list");

        Value *xs[2];
        xs[0] = vs; xs[1] = NULL;

        Value *v = fn->apply(xs);

        vs->rem_ref();
        fn->rem_ref();

        return v;
    }
}

Value* MatrixExp::valueOf(Environment *env) {
    Value *v = list->valueOf(env);
    if (!v) return NULL;
    else if (typeid(*v) != typeid(ListVal)) {
        v->rem_ref();
        return NULL;
    }
    ListVal *array = (ListVal*) v;

    if (array->get()->size() == 0) {
        // We will not permit arrays with 0 rows
        v->rem_ref();
        return NULL;
    }

    List<Value*> *arr = array->get();
    int R = array->get()->size();
    int C = 0;
    
    // We must verify that the outcome is authentic
    auto it = arr->iterator();
    for (int i = 0; i < R; i++) {
        v = it->next(); // Get the next "row"
        if (typeid(*v) != typeid(ListVal)) {
            // Ensure that the list is a 2D list
            array->rem_ref();
            delete it;
            return NULL;
        }
        
        List<Value*> *row = ((ListVal*) v)->get();
        if (i && row->size() != C) {
            // Ensure the array is square
            array->rem_ref();
            delete it;
            return NULL;
        } else if (!i) {
            C = ((ListVal*) v)->get()->size();
            if (C == 0) {
                array->rem_ref();
                delete it;
                return NULL;
            }
        }
        
        // Verify that the row only contains numbers
        auto jt = row->iterator();
        while (jt->hasNext()) {
            v = jt->next();
            if (typeid(*v) != typeid(RealVal) && typeid(*v) != typeid(IntVal)) {
                delete it;
                delete jt;
                array->rem_ref();
                return NULL;
            }
        }

    }
    delete it;

    float *vals = new float[R*C];

    int k = 0;
    it = arr->iterator();
    for (int i = 0; i < R; i++) {
        ListVal *rowval = ((ListVal*) it->next());
        List<Value*> *row = rowval->get();

        auto jt = row->iterator();
        for (int j = 0; j < C; j++, k++) {
            Value *f = jt->next();

            if (typeid(*f) == typeid(RealVal)) vals[k] = ((RealVal*) f)->get();
            else if (typeid(*f) == typeid(IntVal)) vals[k] = (float) ((IntVal*) f)->get();
        }
        delete jt;
    }
    delete it;

    array->rem_ref();

    Matrix m(R, C, vals);
    
    v = new MatrixVal(m);

    return v;
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

Value* PrintExp::valueOf(Environment *env) {
    string s = "";
    
    for (int i = 0; args[i]; i++) {
        Value *v = args[i]->valueOf(env);
        if (!v)
            return NULL;

        if (i) s += " ";
        s += v->toString();
        
        v->rem_ref();
    }

    std::cout << s << "\n";
    return new VoidVal;
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

