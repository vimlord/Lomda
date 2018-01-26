#include "interp.hpp"

#include "expression.hpp"
#include "value.hpp"
#include "environment.hpp"

#include "bnf.hpp"
#include "config.hpp"

#include "expressions/derivative.hpp"

#include <cstring>
#include <cstdlib>
#include <cmath>

using namespace std;

Val run(string program) {
    Exp exp = compile(program);
    if (exp) {
        // If optimization is requested, grant it.
        if (OPTIMIZE()) {
            exp = exp->optimize();
            throw_debug("postprocessor", "program '" + program + "' optimized to '" + exp->toString() + "'");
        }

        Env env = new EmptyEnv();

        throw_debug("init", "initial env Γ := " + env->toString());

        Val val = exp->valueOf(env);
        delete exp;
        env->rem_ref();
        //if (!val) throw_err("runtime", "could not evaluate expression");
        return val;
    } else {
        return NULL;
    }
}

Val ApplyExp::valueOf(Env env) {
    Val f = op->valueOf(env);

    // Null check the function
    if (!f)
        return NULL;
    else
        f = unpack_thunk(f);
    
    // Type check the function
    if (typeid(*f) != typeid(LambdaVal)) {
        throw_type_err(op, "lambda");
        return NULL;
    }
    LambdaVal *F = (LambdaVal*) f;

    int argc = 0;
    while (args[argc]) argc++;

    // Operate on each argument under the given environment
    Val *xs = new Val[argc+1];
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
    Val y = F->apply(xs);
    
    // Garbage collection on the array
    while (i--) xs[i]->rem_ref();
    delete[] xs;

    // And the function
    F->rem_ref();

    return y;
}

Val CastExp::valueOf(Env env) {
    Val val = exp->valueOf(env);
    val = unpack_thunk(val);

    Val res = NULL;

    if (!val) return NULL;
    else if (type == "string")
        res = new StringVal(val->toString());
    else if (type == "int" || type == "integer") {
        // Type conversion to an integer
        if (typeid(*val) == typeid(IntVal))
            return val;
        else if (typeid(*val) == typeid(RealVal))
            res = new IntVal(((RealVal*) val)->get());
        else if (typeid(*val) == typeid(BoolVal))
            res = new IntVal(((BoolVal*) val)->get());
        else if (typeid(*val) == typeid(StringVal)) {
            // String parsing (use BNF parser source)
            string s = ((StringVal*) val)->get();
            parsed_int p = parseInt(s);
            if (p.len >= 0 && parseSpaces(s.substr(p.len)) + p.len == s.length())
                res = new IntVal(p.item);
        }
    } else if (type == "real") {
        // Type conversion to a float
        if (typeid(*val) == typeid(IntVal))
            res = new RealVal(((IntVal*) val)->get());
        else if (typeid(*val) == typeid(RealVal))
            return val;
        else if (typeid(*val) == typeid(BoolVal))
            res = new RealVal(((BoolVal*) val)->get());
        else if (typeid(*val) == typeid(StringVal)) {
            // String parsing
            string s = ((StringVal*) val)->get();
            parsed_float p = parseFloat(s);
            if (p.len >= 0 && parseSpaces(s.substr(p.len)) + p.len == s.length())
                res = new RealVal(p.item);
        }
    } else if (type == "bool" || type == "boolean") {
        // Type conversion to a boolean
        if (typeid(*val) == typeid(IntVal))
            res = new BoolVal(((IntVal*) val)->get());
        else if (typeid(*val) == typeid(RealVal))
            res = new BoolVal(((RealVal*) val)->get());
        else if (typeid(*val) == typeid(BoolVal))
            return val;
        else if (typeid(*val) == typeid(StringVal)) {
            // String parsing
            string s = ((StringVal*) val)->get();
            int i;
            if ((i = parseLit(s, "true")) >= 0 && i + parseSpaces(s.substr(i)) == s.length())
                res = new BoolVal(true);
            else if ((i = parseLit(s, "false")) >= 0 && i + parseSpaces(s.substr(i)) == s.length())
                res = new BoolVal(false);
        }
    } else {
        throw_err("type", "type " + type + " is not a castable type");
        val->rem_ref();
        return NULL;
    }
    
    if (!res)
        throw_err("type", "expression '" + exp->toString() + "' cannot be casted to " + type);

    val->rem_ref();
    return res;
}

Val DerivativeExp::valueOf(Env env) {

    // Base case: we know of nothing
    Env denv = new EmptyEnv;

    // Perform initial loading
    LinkedList<ExtendEnv*> env_frames;
    Env tmp = env;
    while (typeid(*tmp) == typeid(ExtendEnv)) {
        env_frames.add(0, (ExtendEnv*) tmp);
        tmp = tmp->subenvironment();
    }
    
    while (!env_frames.isEmpty()) {
        ExtendEnv *ee = env_frames.remove(0);
        Val v = ee->topVal();
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
            // Trivial derivative: d/dx c = 0, d/dx x = x
            int c = id == var ? 1 : 0;
            
            Val V = env->apply(var);
            v = V ? deriveConstVal(v, V, c) : NULL;

            if (v) {
                // The value is of a differentiable type
                denv = new ExtendEnv(id, v, denv);
                v->rem_ref(); // The derivative exists only within the environment
                throw_debug("calc_init", "d/d" + var + " " + id + " = " + v->toString());
            }
        }
    }

    throw_debug("calc_init", "computing " + toString() + " under:\n"
               +"           Γ := " + env->toString() + "\n"
               +"           dΓ/d" + var + " := " + denv->toString());

    // Now, we have the variable, the environment, and the differentiable
    // environment. So, we can simply derive and return the result.
    //std::cout << "compute d/d" << var << " | env ::= " << *env << ", denv ::= " << *denv << "\n";
    Val res = func->derivativeOf(var, env, denv);

    // Garbage collection
    denv->rem_ref();

    return res;
}

Val FalseExp::valueOf(Env env) { return new BoolVal(false); }

Val FoldExp::valueOf(Env env) {
    Val lst = list->valueOf(env);
    lst = unpack_thunk(lst);

    if (!lst) return NULL;
    else if (typeid(*lst) != typeid(ListVal)) {
        lst->rem_ref();
        throw_type_err(list, "list");
        return NULL;
    }
    
    Val f = func->valueOf(env);
    f = unpack_thunk(f);

    if (!f) return NULL;
    else if (typeid(*f) != typeid(LambdaVal)) {
        throw_type_err(func, "lambda");
        lst->rem_ref();
        return NULL;
    }

    LambdaVal *fn = (LambdaVal*) f;
    int i;
    for (i = 0; fn->getArgs()[i] != ""; i++);
    if (i != 2) {
        throw_err("runtime", "function defined by '" + func->toString() + "' does not take exactly two arguments");
        lst->rem_ref();
        f->rem_ref();
        return NULL;
    }

    auto it = ((ListVal*) lst)->get()->iterator();
    Val xs[3];
    xs[0] = base->valueOf(env); // First slot is the accumulator
    xs[2] = NULL; // Last slot is a null terminator.

    while (it->hasNext() && xs[0]) {
        // Second slot is the element
        xs[1] = it->next();
        
        // Update the accumulator.
        Val acc = fn->apply(xs);
        xs[0]->rem_ref();
        xs[0] = acc;
    }
    
    delete it;

    fn->rem_ref();
    lst->rem_ref();

    return xs[0];
}

Val ForExp::valueOf(Env env) {
    // Evaluate the list
    Val listExp = set->valueOf(env);
    listExp = unpack_thunk(listExp);

    if (!listExp) return NULL;
    else if (typeid(*listExp) != typeid(ListVal)) {
        throw_type_err(set, "list");
        return NULL;
    }
    List<Val> *list = ((ListVal*) listExp)->get();
    
    // Gather an iterator
    Iterator<int, Val> *it = list->iterator();

    // Value to be return
    Val v = new VoidVal;

    while (it->hasNext()) {
        // Get the next item from the list
        Val x = it->next();
        
        // Build an environment
        env->add_ref();
        Env e = new ExtendEnv(id, x, env);

        v->rem_ref();
        v = body->valueOf(e);
        
        e->rem_ref();
    }

    delete it;

    return v;

}

Val IfExp::valueOf(Env env) {
    Val b = cond->valueOf(env);
    b = unpack_thunk(b);
    
    if (!b) return NULL;
    else if (typeid(*b) != typeid(BoolVal)) {
        throw_type_err(cond, "boolean");
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

Val InputExp::valueOf(Env env) {
    string s;
    getline(cin, s);
    return new StringVal(s);
}

Val IntExp::valueOf(Env env) {
    return new IntVal(val);
}

Val IsaExp::valueOf(Env env) {
    Val val = exp->valueOf(env);
    val = unpack_thunk(val);
    if (!val) return NULL;
    
    // Compare the object type with the sought type
    bool res;
    if (typeid(*val) == typeid(IntVal))
        res = type == "int" || type == "integer" || type == "number";
    else if (typeid(*val) == typeid(RealVal))
        res = type == "real" || type == "number";
    else if (typeid(*val) == typeid(ListVal))
        res = type == "list";
    else if (typeid(*val) == typeid(LambdaVal))
        res = type == "function" || type == "lambda";
    else if (typeid(*val) == typeid(StringVal))
        res = type == "string";
    else
        res = false;

    return new BoolVal(res);
}

Val LambdaExp::valueOf(Env env) {
    int argc;
    for (argc = 0; xs[argc] != ""; argc++);

    string *ids = new string[argc+1];
    ids[argc] = "";
    while (argc--) ids[argc] = xs[argc];
    
    env->add_ref();
    return new LambdaVal(ids, exp->clone(), env);
}

Val LetExp::valueOf(Env env) {
    // We will operate on a clone
    env = env->clone();

    int argc = 0;
    for (; exps[argc]; argc++);
    throw_debug("env", "adding " + to_string(argc) + " vars to env Γ := " + env->toString());

    // I want to make all of the lambdas recursive.
    // So, I will track my lambdas for now
    LinkedList<LambdaVal*> lambdas;

    // There is now one more reference to the original environment
    env->add_ref();
    
    // Extend the environment
    for (int i = 0; i < argc; i++) {
        // Compute the expression
        Val v = exps[i]->valueOf(env);
        if (!v) {
            // Garbage collection will happen here
            env->rem_ref();
            return NULL;
        }
        
        // Add it to the environment
        Val x = v->clone();
        env = new ExtendEnv(ids[i], x, env);
        
        // Drop references
        v->rem_ref();
        x->rem_ref();

        // We permit all lambdas to have recursive behavior
        if (typeid(*x) == typeid(LambdaVal))
            lambdas.add(0, (LambdaVal*) x);
    }
    
    // Apply recursive principles
    while (!lambdas.isEmpty()) {
        LambdaVal *v = lambdas.remove(0);
        v->setEnv(env);
    }

    // Display the env if necessary
    throw_debug("env", "original env Γ extended to env Γ' := " + env->toString());

    // Compute the result
    Val y = body->valueOf(env);

    // Garbage collection
    env->rem_ref();
        
    // Return the result
    return y;

}

Val ListExp::valueOf(Env env) {
    // Generate a blank list
    ListVal *val = new ListVal;
    
    // Add each item
    auto it = list->iterator();
    for(int i = 0; it->hasNext(); i++) {
        // Compute the value of each item
        Val v = it->next()->valueOf(env);
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

Val ListAccessExp::valueOf(Env env) {
    // Get the list
    Val f = list->valueOf(env);
    f = unpack_thunk(f);

    if (!f)
        return NULL;
    else if (typeid(*f) != typeid(ListVal) && typeid(*f) != typeid(StringVal)) {
        throw_type_err(list, "list or string");
        return NULL;
    }
    
    // Get the index
    Val index = idx->valueOf(env);
    index = unpack_thunk(index);

    if (!index) return NULL;
    else if (typeid(*index) != typeid(IntVal)) {
        throw_type_err(idx, "integer");
        return NULL;
    }
    int i = ((IntVal*) index)->get();
    
    // GC of the index
    index->rem_ref();

    if (typeid(*f) == typeid(ListVal)) {
        // The list
        List<Val> *vals = ((ListVal*) f)->get();
        
        // Bound check
        if (i < 0 || i >= vals->size()) {
            throw_err("runtime", "index " + to_string(i) + " is out of bounds (len: " + to_string(vals->size()) + ")");
            f->rem_ref();
            return NULL;
        }
    
        // Get the item
        Val v = vals->get(i);

        v->add_ref();
        f->rem_ref();

        return v;
    
    } else {
        // The string
        string s = f->toString();

        // Bound check
        if (i < 0 || i >= s.length()) {
            throw_err("runtime", "index " + to_string(i) + " is out of bounds (len: " + to_string(s.length()) + ")");
            f->rem_ref();
            return NULL;
        }

        s = s.substr(i, 1);

        f->rem_ref();
        
        return new StringVal(s);
    }
}

Val ListAddExp::valueOf(Env env) {
    // Get the list
    Val f = list->valueOf(env);
    f = unpack_thunk(f);

    if (!f)
        return NULL;
    else if (typeid(*f) != typeid(ListVal)) {
        throw_type_err(list, "list");
        return NULL;
    }
    List<Val> *vals = ((ListVal*) f)->get();

    // Compute the index
    Val index = idx->valueOf(env);
    index = unpack_thunk(index);

    if (!index) return NULL;
    else if (typeid(*index) != typeid(IntVal)) {
        throw_type_err(idx, "integer");
        return NULL;
    }
    int i = ((IntVal*) index)->get();

    // Compute the value
    Val val = elem->valueOf(env);
    if (!index)
        return NULL;
    
    // Remove and return the item
    vals->add(i, val);

    return new VoidVal;
}

Val ListRemExp::valueOf(Env env) {
    // Get the list
    Val f = list->valueOf(env);
    f = unpack_thunk(f);

    if (!f)
        return NULL;
    else if (typeid(*f) != typeid(ListVal)) {
        throw_type_err(list, "list");
        return NULL;
    }
    List<Val> *vals = ((ListVal*) f)->get();

    // Compute the index
    Val index = idx->valueOf(env);
    index = unpack_thunk(index);

    if (!index) return NULL;
    else if (typeid(*index) != typeid(IntVal)) {
        throw_type_err(idx, "integer");
        return NULL;
    }
    int i = ((IntVal*) index)->get();
    
    // Remove and return the item
    return vals->remove(i);
}

Val ListSliceExp::valueOf(Env env) {
    // Get the list
    Val lst = list->valueOf(env);
    lst = unpack_thunk(lst);

    if (!lst)
        return NULL;
    else if (typeid(*lst) != typeid(ListVal) && typeid(*lst) != typeid(StringVal)) {
        throw_type_err(list, "list or string");
        lst->rem_ref(); // Garbage collection
        return NULL;
    }
    
    // Get the index
    Val f = from->valueOf(env);
    f = unpack_thunk(f);

    if (!f) return NULL;
    else if (typeid(*f) != typeid(IntVal)) {
        throw_type_err(from, "integer");
        lst->rem_ref(); // Garbage collection
        f->rem_ref();
        return NULL;
    }

    int i = ((IntVal*) f)->get();
    f->rem_ref();

    // Get the index
    Val t = to->valueOf(env);
    t = unpack_thunk(t);

    if (!t) return NULL;
    else if (typeid(*t) != typeid(IntVal)) {
        throw_type_err(to, "integer");
        lst->rem_ref(); // Garbage collection
        t->rem_ref();
        return NULL;
    }

    int j = ((IntVal*) t)->get();
    t->rem_ref();
    
    if (typeid(*lst) == typeid(ListVal)) {
        // The list
        List<Val> *vals = ((ListVal*) lst)->get();

        if (i < 0 || j < 0 || i >= vals->size() || j > vals->size()) {
            throw_err("runtime", "index " + to_string(i) + " is out of bounds (len: " + to_string(vals->size()) + ")");
            lst->rem_ref();
            return NULL;
        }

        // Get the item
        LinkedList<Val> *vs = new LinkedList<Val>;
        
        auto it = vals->iterator();
        int x;
        for (x = 0; x < i; x++) it->next();
        for (;x < j && it->hasNext(); x++) {
            // Add the value and a reference to it
            Val v = it->next();
            vs->add(x-i, v);
            v->add_ref();
        }
        
        // Garbage collection
        lst->rem_ref();
        delete it;

        return new ListVal(vs);
    } else {
        // String
        string s = lst->toString();

        if (i < 0 || j < 0 || i >= s.length() || j > s.length()) {
            throw_err("runtime", "index " + to_string(i) + " is out of bounds (len: " + to_string(s.length()) + ")");
            lst->rem_ref();
            return NULL;
        }

        lst->rem_ref();

        return new StringVal(s.substr(i, j-i));
    }
}

Val MagnitudeExp::valueOf(Env env) {
    Val v = exp->valueOf(env);
    Val res = NULL;

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
        res = new IntVal(val);
    }
    
    // Garbage collection
    v->rem_ref();

    return res;
}

Val MapExp::valueOf(Env env) { 
    Val f = func->valueOf(env);
    f = unpack_thunk(f);
    if (!f) return NULL;
    
    if (typeid(*f) != typeid(LambdaVal)) {
        throw_type_err(func, "lambda");
        f->rem_ref();
        return NULL;
    }

    // Extract the function... require that it have one argument
    LambdaVal *fn = (LambdaVal*) f;
    if (fn->getArgs()[0] == "" || fn->getArgs()[1] != "") {
        throw_err("runtime", "map function '" + fn->toString() + "' does not take exactly one argument");
        fn->rem_ref();
        return NULL;
    }
    
    Val vs = list->valueOf(env);
    vs = unpack_thunk(vs);
    if (!vs) {
        fn->rem_ref();
        return NULL;
    }

    // Get the arguments
    Exp map = fn->getBody();

    if (typeid(*vs) == typeid(ListVal)) {
        // Given a list, map each element of the list
        ListVal *vals = (ListVal*) vs;
        ListVal *res = new ListVal();
        
        int i = 0;
        auto it = vals->get()->iterator();
        while (it->hasNext()) {
            Val v = it->next();

            Val *xs = new Val[2];
            xs[0] = v;
            xs[1] = NULL;

            Val elem = fn->apply(xs);

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

    } else if (WERROR()) {
        throw_err("runtime", "expression '" + list->toString() + " does not evaluate as list");
        vs->rem_ref();
        fn->rem_ref();
        return NULL;
    } else {
        throw_warning("runtime", "expression '" + list->toString() + " does not evaluate as list");

        Val xs[2];
        xs[0] = vs; xs[1] = NULL;

        Val v = fn->apply(xs);

        vs->rem_ref();
        fn->rem_ref();

        return v;
    }
}

Val sqnorm(Val v, Env env) {
    if (typeid(*v) == typeid(IntVal)) {
        // Magnitude of number is its absolute value
        int val = ((IntVal*) v)->get();
        return new IntVal(val * val);
    } else if (typeid(*v) == typeid(RealVal)) {
        // Magnitude of number is its absolute value
        int val = ((RealVal*) v)->get();
        return new RealVal(val * val);
    } else if (typeid(*v) == typeid(ListVal)) {
        // Magnitude of list is its length
        auto it = ((ListVal*) v)->get()->iterator();

        float sum = 0;
        
        while (it->hasNext()) {
            Val v = sqnorm(it->next(), env);

            if (!v) return NULL;

            auto x = typeid(*v) == typeid(IntVal)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
            v->rem_ref();

            sum += x;
        }

        return new RealVal(sum);

    } else return NULL;
}
Val NormExp::valueOf(Env env) {
    Val val = exp->valueOf(env);
    val = unpack_thunk(val);
    if (!val) return NULL;

    Val v = sqnorm(val, env);
    val->rem_ref();
    if (!v) return NULL;

    auto x = typeid(*v) == typeid(IntVal)
            ? ((IntVal*) v)->get()
            : ((RealVal*) v)->get();

    v->rem_ref();
    
    return x < 0 ? NULL : new RealVal(sqrt(x));
}


Val NotExp::valueOf(Env env) {
    Val v = exp->valueOf(env);
    v = unpack_thunk(v);
    
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

Val OperatorExp::valueOf(Env env) {
    Val a = left->valueOf(env);
    a = unpack_thunk(a);

    if (!a) return NULL;

    Val b = right->valueOf(env);
    b = unpack_thunk(b);

    if (!b) {
        a->rem_ref();
        return NULL;
    } else {
        Val res = op(a, b);
        a->rem_ref();
        b->rem_ref();
        return res;
    }
}

Val PrintExp::valueOf(Env env) {
    string s = "";
    
    for (int i = 0; args[i]; i++) {
        Val v = args[i]->valueOf(env);
        if (!v)
            return NULL;

        if (i) s += " ";
        s += v->toString();
        
        v->rem_ref();
    }

    std::cout << s << "\n";
    return new VoidVal;
}

Val RealExp::valueOf(Env env) {
    return new RealVal(val);
}

Val SequenceExp::valueOf(Env env) {
    Val v = NULL;
    
    // For each expression in the sequence...
    auto it = seq->iterator();
    do {
        // Compute it and store it.
        if (v) v->rem_ref();
        v = it->next()->valueOf(env);
    } while (it->hasNext() && v); // End the loop early if an error occurs.
    
    // Return the final outcome, or NULL if one of
    // the executions fails.
    delete it;
    return v;
}

Val SetExp::valueOf(Env env) {
    Val v = NULL;

    if (typeid(*exp) == typeid(ListAccessExp)) {
        ListAccessExp *acc = (ListAccessExp*) tgt;
        
        Val u = acc->getList()->valueOf(env);
        if (!u) {
            return NULL;
        } else if (typeid(*u) != typeid(ListVal)) {
            u->rem_ref();
            return NULL;
        }

        ListVal *lst = (ListVal*) u;

        Val index = acc->getIdx()->valueOf(env);
        if (!index) {
            u->rem_ref();
            return NULL;
        }

        if (typeid(*index) != typeid(IntVal)) {
            index->rem_ref();
            u->rem_ref();
            return NULL;
        }

        int idx = ((IntVal*) index)->get();
        index->rem_ref();
        
        if (idx < 0 || lst->get()->size() <= idx) {
            u->rem_ref();
            return NULL;
        }
        
        if (!(v = exp->valueOf(env))) {
            u->rem_ref();
            return NULL;
        } else {
            lst->get()->remove(idx)->rem_ref();
            lst->get()->add(idx, v);
        }

    } else if (typeid(*exp) == typeid(VarExp)) {
        VarExp *var = (VarExp*) tgt;
        
        v = exp->valueOf(env);
        if (!v) return NULL;
        
        env->set(var->toString(), v);

    } else {
        // Get info for modifying the environment
        Val u = tgt->valueOf(env);
        if (!u)
            // The variable doesn't exist
            return NULL;

        // Evaluate the expression
        v = exp->valueOf(env);

        if (!v)
            // The value could not be evaluated.
            return NULL;
        
        // Set the new value
        if (u->set(v)) {
            v->rem_ref();
            return NULL;
        }
    }
    
    v->add_ref();
    return v;
}

Val StdlibOpExp::valueOf(Env env) {
    Val v = x->valueOf(env);
    v = unpack_thunk(v);

    if (!v) return NULL;
    
    if (
        typeid(*v) == typeid(LambdaVal)
    ||  typeid(*v) == typeid(ListVal)
    ||  typeid(*v) == typeid(StringVal)
    ) {
        throw_type_err(x, "numerical");
        return NULL;
    }

    auto z = typeid(*v) == typeid(IntVal)
            ? ((IntVal*) v)->get()
            : ((RealVal*) v)->get();
    v->rem_ref();
    
    switch (op) {
        case SIN: return new RealVal(sin(z));
        case COS: return new RealVal(cos(z));
        case LOG: return new RealVal(log(z));
        case SQRT: return new RealVal(sqrt(z));
    }
}

Val TrueExp::valueOf(Env env) { return new BoolVal(true); }

Val VarExp::valueOf(Env env) {
    Val res = env->apply(id);
    if (!res) {
        throw_err("runtime", "variable '" + id + "' was not recognized");
        if (VERBOSITY()) throw_debug("runtime error", "error ocurred w/ scope:\n" + env->toString());
        return NULL;
    } else {
        res->add_ref(); // This necessarily creates a new reference. So, we must track it.
        return res;
    }
}

Val WhileExp::valueOf(Env env) {
    bool skip = alwaysEnter;
    Val v = new VoidVal;

    while (true) {
        Val c = cond->valueOf(env);
        c = unpack_thunk(c);

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

