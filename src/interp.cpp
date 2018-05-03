#include "interp.hpp"

#include "expression.hpp"
#include "value.hpp"
#include "environment.hpp"

#include "bnf.hpp"
#include "config.hpp"

#include "types.hpp"

#include "proof.hpp"

#include "expressions/derivative.hpp"

#include "stdlib.hpp"

#include <cstring>
#include <cstdlib>

// For math stuff
#include <cmath>
#include "math.hpp"

#include <fstream>

using namespace std;

Val run(string program) {
    Exp exp = compile(program);
    
    if (exp) {
        
        throw_debug("postprocessor", "performing verification of '" + exp->toString() + "'");
        Trie<bool> *vardta = new Trie<bool>;
        bool valid = exp->postprocessor(vardta);
        delete vardta;

        if (!valid)
            return NULL;
        
        if (USE_TYPES()) {
            // Use the type system.
            show_proof_step("We seek to find t such that {} ⊢ " + exp->toString() + " : t");

            // For consistency, we need to reset the type variable names.
            TypeEnv::reset_tvars();
            
            // Define a typing environment.
            Tenv tenv = new TypeEnv;

            // Type the expression.
            Type* type = exp->typeOf(tenv);

            if (type) {
                show_proof_step("Quod erat demonstrandum (QED).");
                std::cout << *tenv << " ⊢ " << *exp << " : " << *type << "\n";
                delete type;
            } else {
                show_proof_step("Reductio ad absurdum.");
                
                auto s = tenv->toString() + " ⊢ " + exp->toString() + " is untypable";
                if (USE_TYPES() > 1)
                    throw_err("type", s);
                else
                    std::cout << s << "\n";
            }

            delete tenv;

            if (!type && USE_TYPES() > 1)
                return NULL;
        }

        // If optimization is requested, grant it.
        if (OPTIMIZE()) {
            exp = exp->optimize();
            throw_debug("postprocessor", "program '" + program + "' optimized to '" + exp->toString() + "'");
        }

        Env env = new EmptyEnv();

        throw_debug("init", "initial env Γ := " + env->toString());

        Val val = exp->evaluate(env);
        delete exp;
        env->rem_ref();
        //if (!val) throw_err("runtime", "could not evaluate expression");
        return val;
    } else {
        return NULL;
    }
}

Val ApplyExp::evaluate(Env env) {
    Val f = op->evaluate(env);

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
        xs[i] = args[i]->evaluate(env);

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

Val CastExp::evaluate(Env env) {
    Val val = exp->evaluate(env);
    val = unpack_thunk(val);

    Val res = NULL;

    if (!val) return NULL;
    else if (isType<StringType>(type))
        res = new StringVal(val->toString());
    else if (isType<IntType>(type)) {
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
    } else if (isType<RealType>(type)) {
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
    } else if (isType<BoolType>(type)) {
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
        throw_err("type", "type " + type->toString() + " is not a castable type");
        val->rem_ref();
        return NULL;
    }
    
    if (!res)
        throw_err("type", "expression '" + exp->toString() + "' cannot be casted to " + type->toString());

    val->rem_ref();
    return res;
}

Val DerivativeExp::evaluate(Env env) {

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
                            lv->getBody()->symb_diff(var)
                            , lv->getEnv()->clone());

            // Lambda derivative: d/dx lambda (x) f(x) = lambda (x) d/dx f(x)
            // We will need the derivative for this
            denv = new ExtendEnv(id, dv, denv);
            dv->rem_ref(); // The derivative exists only within the environment
        } else {
            // Trivial derivative: d/dx c = 0, d/dx x = x
            int c = id == var ? 1 : 0;
            
            Val V = env->apply(var);
            v = V ? deriveConstVal(var, v, V, c) : NULL;

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

Val DictExp::evaluate(Env env) {
    // Verify that none of the keys repeat
    auto it = keys->iterator();
    for (int i = 0; it->hasNext(); i++) {
        string x = it->next();

        auto jt = keys->iterator();
        for (int j = 0; j <= i; j++) jt->next();
        while (jt->hasNext()) {
            if (jt->next() == x) {
                throw_err("runtime", "keys should not be repeated in dictionary definition; see: " + toString());
                delete it;
                delete jt;
                return NULL;
            }
        }
        delete jt;
    }

    delete it;
    
    Trie<Val> *vs = new Trie<Val>;

    DictVal *res = new DictVal(vs);
    
    auto kt = keys->iterator();
    auto vt = vals->iterator();

    while (kt->hasNext()) {
        Val v = vt->next()->evaluate(env);
        if (!v) {
            delete res;
            return NULL;
        }

        string k = kt->next();

        vs->add(k, v);
    }

    return res;

}

Val FalseExp::evaluate(Env env) { return new BoolVal(false); }

Val FoldExp::evaluate(Env env) {
    Val lst = list->evaluate(env);
    lst = unpack_thunk(lst);

    if (!lst) return NULL;
    else if (typeid(*lst) != typeid(ListVal)) {
        lst->rem_ref();
        throw_type_err(list, "list");
        return NULL;
    }
    
    Val f = func->evaluate(env);
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
    xs[0] = base->evaluate(env); // First slot is the accumulator
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

Val ForExp::evaluate(Env env) {
    // Evaluate the list
    Val listExp = set->evaluate(env);
    listExp = unpack_thunk(listExp);

    if (!listExp) return NULL;
    else if (typeid(*listExp) != typeid(ListVal)) {
        throw_type_err(set, "list");
        return NULL;
    }
    List<Val> *list = ((ListVal*) listExp)->get();
    
    // Gather an iterator
    auto it = list->iterator();

    while (it->hasNext()) {
        // Get the next item from the list
        Val x = it->next();
        
        // Build an environment
        env->add_ref();
        Env e = new ExtendEnv(id, x, env);

        Val v = body->evaluate(e);  
        e->rem_ref();

        if (!v) {
            delete it;
            listExp->rem_ref();
            return NULL;
        }
    }

    delete it;
    listExp->rem_ref();

    return new VoidVal;

}

Val HasExp::evaluate(Env env) {
    CompareExp equals(NULL, NULL, EQ);
    Val res = NULL;

    Val x = item->evaluate(env);
    if (!x) return NULL;

    Val xs = set->evaluate(env);
    if (!xs) {
        x->rem_ref();
        return NULL;
    } else if (val_is_list(xs)) {
        List<Val> *lst = ((ListVal*) xs)->get();
        
        auto it = lst->iterator();
        while (!res && it->hasNext()) {
            Val v = it->next();
            
            Val b = equals.op(x, v);
            if (!b) {
                delete x;
                delete xs;
                delete it;
                return NULL;
            } else if (((BoolVal*) b)->get())
                res = new BoolVal(true);
        }
        if (!res) res = new BoolVal(false);

        delete it;
    } else if (val_is_dict(xs)) {

        if (val_is_string(x)) {
            string key = x->toString();
            
            auto it = ((DictVal*) xs)->getVals()->iterator();

            while (!res && it->hasNext())
                if (it->next() == key)
                    res = new BoolVal(true);

            if (!res) res = new BoolVal(false);

            delete it;
        } else
            throw_type_err(item, "string");
    } else
        throw_type_err(set, "list or dictionary");

    delete x;
    delete xs;

    return res;
}

Val IfExp::evaluate(Env env) {
    Val b = cond->evaluate(env);
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
        return tExp->evaluate(env);
    else
        return fExp->evaluate(env);
}

Val ImportExp::evaluate(Env env) {

    // First, we will attempt to load it from standard libraries
    Val mod = load_stdlib(module);
    
    if (mod) {
        throw_debug("module IO", "loaded stdlib " + module);
    } else {
        // Attempt to open the file
        ifstream file;
        file.open("./" + module + ".lom");
        
        // Ensure that the file exists
        if (!file) {
            throw_err("IO", "could not load module " + module);
            return NULL;
        }
        
        // Read the program from the input file
        int i = 0;
        string program = "";
        do {
            string s;
            getline(file, s);

            if (i++) program += " ";
            program += s;
        } while (file);

        throw_debug("module IO", "module " + module + " is defined by '" + program + "'\n");
        file.close();
        
        mod = run(program);
    }

    if (mod) {
        throw_debug("module", "module " + module + " := " + mod->toString());
        
        // We will extend the environment with the newly found module
        env->add_ref();
        env = new ExtendEnv(name, mod, env);
        
        // Evaluate the subexpression.
        Val v = exp->evaluate(env);

        // Garbage collection
        env->rem_ref();

        return v;
    } else {
        throw_err("module", "module " + module + " could not be evaluated");
        return NULL;
    }

}

Val InputExp::evaluate(Env env) {
    string s;
    getline(cin, s);
    return new StringVal(s);
}

Val IntExp::evaluate(Env env) {
    return new IntVal(val);
}

bool static_typecheck(Val val, Type *type) {
    if (typeid(*val) == typeid(IntVal))
        return isType<IntType>(type) || isType<RealType>(type);
    else if (typeid(*val) == typeid(RealVal))
        return isType<RealType>(type) && (!isType<IntType>(type) || isType<VarType>(type));
    else if (typeid(*val) == typeid(BoolVal))
        return isType<BoolType>(type);
    else if (typeid(*val) == typeid(StringVal))
        return isType<StringType>(type);
    else if (typeid(*val) == typeid(TupleVal)) {
        // Type each side if it is a tuple.
        return isType<TupleType>(type) &&
            static_typecheck(((TupleVal*) val)->getLeft(), ((TupleType*) type)->getLeft())
        &&  static_typecheck(((TupleVal*) val)->getRight(), ((TupleType*) type)->getRight());
    } else if (typeid(*val) == typeid(ListVal)) {
        if (isType<ListType>(type)) {
            // We must check each element for correctness.
            auto T = ((ListType*) type)->subtype();
            auto it = ((ListVal*) val)->get()->iterator();
            bool res = true;
            while (res && it->hasNext())
                res = static_typecheck(it->next(), T);
            return res;
        } else
            return false;
    } else
        return false;
}
Val IsaExp::evaluate(Env env) {
    Val val = exp->evaluate(env);
    val = unpack_thunk(val);
    if (!val) return NULL;
    
    // Compare the object type with the sought type
    auto res = new BoolVal(static_typecheck(val, type));
    
    // GC
    val->rem_ref();

    return res;
}

Val LambdaExp::evaluate(Env env) {
    int argc;
    for (argc = 0; xs[argc] != ""; argc++);

    string *ids = new string[argc+1];
    ids[argc] = "";
    while (argc--) ids[argc] = xs[argc];
    
    env->add_ref();
    return new LambdaVal(ids, exp->clone(), env);
}

Val LetExp::evaluate(Env env) {
    int argc = 0;
    for (; exps[argc]; argc++);
    throw_debug("env", "adding " + to_string(argc) + " vars to env Γ := " + env->toString());

    // I want to make all of the lambdas recursive.
    // So, I will track my lambdas for now
    LinkedList<LambdaVal*> lambdas;

    env->add_ref();

    // Extend the environment
    for (int i = 0; i < argc; i++) {
        // Compute the expression
        Val v = exps[i]->evaluate(env);
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

        // We permit lambdas that request recursion to have it.
        if (rec && rec[i] && typeid(*x) == typeid(LambdaVal))
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
    Val y = body->evaluate(env);

    // Garbage collection
    env->rem_ref();
        
    // Return the result
    return y;

}

Val ListExp::evaluate(Env env) {
    // Generate a blank list
    ListVal *val = new ListVal;
    
    // Add each item
    auto it = list->iterator();
    for(int i = 0; it->hasNext(); i++) {
        // Compute the value of each item
        Val v = it->next()->evaluate(env);
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

Val ListAccessExp::evaluate(Env env) {
    // Get the list
    Val f = list->evaluate(env);
    f = unpack_thunk(f);

    if (!f)
        return NULL;
    else if (typeid(*f) != typeid(ListVal)) {
        throw_type_err(list, "list");
        return NULL;
    }
    
    // Get the index
    Val index = idx->evaluate(env);
    index = unpack_thunk(index);

    if (!index) return NULL;  

    // The list
    List<Val> *vals = ((ListVal*) f)->get();

    if (typeid(*index) != typeid(IntVal)) {
            throw_type_err(idx, "integer");
            f->rem_ref();
            return NULL;
    }
    int i = ((IntVal*) index)->get();
    index->rem_ref();
    
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

}

Val DictAccessExp::evaluate(Env env) {
    Val f = list->evaluate(env);
    f = unpack_thunk(f);

    if (!f)
        return NULL;
    else if (typeid(*f) != typeid(DictVal)) {
        throw_type_err(list, "dict, list, or string");
        return NULL;
    } else {
        DictVal *d = (DictVal*) f;
        Val v;

        if (d->getVals()->hasKey(idx)) {
            v = d->getVals()->get(idx);
            v->add_ref();
            return v;
        } else
            v = new VoidVal;

        d->rem_ref();
        
        return v;
    }
}

Val ListAddExp::evaluate(Env env) {
    // Get the list
    Val f = list->evaluate(env);
    f = unpack_thunk(f);

    if (!f)
        return NULL;
    else if (typeid(*f) != typeid(ListVal)) {
        throw_type_err(list, "list");
        return NULL;
    }
    List<Val> *vals = ((ListVal*) f)->get();

    // Compute the index
    Val index = idx->evaluate(env);
    index = unpack_thunk(index);

    if (!index) return NULL;
    else if (typeid(*index) != typeid(IntVal)) {
        throw_type_err(idx, "integer");
        return NULL;
    }
    int i = ((IntVal*) index)->get();

    // Compute the value
    Val val = elem->evaluate(env);
    if (!index)
        return NULL;
    
    // Remove and return the item
    vals->add(i, val);

    return new VoidVal;
}

Val ListRemExp::evaluate(Env env) {
    // Get the list
    Val f = list->evaluate(env);
    f = unpack_thunk(f);

    if (!f)
        return NULL;
    else if (typeid(*f) != typeid(ListVal)) {
        throw_type_err(list, "list");
        return NULL;
    }
    List<Val> *vals = ((ListVal*) f)->get();

    // Compute the index
    Val index = idx->evaluate(env);
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

Val ListSliceExp::evaluate(Env env) {
    // Get the list
    Val lst = list->evaluate(env);
    lst = unpack_thunk(lst);

    if (!lst)
        return NULL;
    else if (typeid(*lst) != typeid(ListVal)) {
        throw_type_err(list, "list");
        lst->rem_ref(); // Garbage collection
        return NULL;
    }
    
    // Get the index
    int i;
    if (from) {
        Val f = from->evaluate(env);
        f = unpack_thunk(f);

        if (!f) return NULL;
        else if (typeid(*f) != typeid(IntVal)) {
            throw_type_err(from, "integer");
            lst->rem_ref(); // Garbage collection
            f->rem_ref();
            return NULL;
        }

        i = ((IntVal*) f)->get();
        f->rem_ref();
    } else
        i = 0;

    int j;
    
    if (to) {
        // Get the index
        Val t = to->evaluate(env);
        t = unpack_thunk(t);

        if (!t) return NULL;
        else if (typeid(*t) != typeid(IntVal)) {
            throw_type_err(to, "integer");
            lst->rem_ref(); // Garbage collection
            t->rem_ref();
            return NULL;
        }

        j = ((IntVal*) t)->get();
        t->rem_ref();
    } else if (typeid(*lst) == typeid(ListVal))
        j = ((ListVal*) lst)->get()->size();
    else
        j = lst->toString().length();
    
    // The list
    ArrayList<Val> *vals = ((ListVal*) lst)->get();

    if (i < 0 || j < 0 || i >= vals->size() || j > vals->size()) {
        throw_err("runtime", "index " + to_string(i) + " is out of bounds (len: " + to_string(vals->size()) + ")");
        lst->rem_ref();
        return NULL;
    }

    // Get the item
    auto vs = new ArrayList<Val>;
    
    auto it = vals->iterator();

    int x;
    for (x = 0; x < i; x++) it->next();
    for (;x < j && it->hasNext(); x++) {
        // Add the value and a reference to it
        Val v = it->next();
        vs->add(vs->size(), v);
        v->add_ref();
    }
    
    // Garbage collection
    lst->rem_ref();
    delete it;

    Val res = new ListVal(vs);

    return res;
}

Val MagnitudeExp::evaluate(Env env) {
    Val v = exp->evaluate(env);
    Val res = NULL;

    if (!v) return NULL;
    else if (typeid(*v) == typeid(IntVal)) {
        // Magnitude of number is its absolute value
        int val = ((IntVal*) v)->get();
        res = new IntVal(val > 0 ? val : -val);
    } else if (typeid(*v) == typeid(RealVal)) {
        // Magnitude of number is its absolute value
        float val = ((RealVal*) v)->get();
        res = new RealVal(val > 0 ? val : -val);
    } else if (typeid(*v) == typeid(ListVal)) {
        // Magnitude of list is its length
        int val = ((ListVal*) v)->get()->size();
        res = new IntVal(val);
    } else if (typeid(*v) == typeid(StringVal)) {
        res = new IntVal(v->toString().length());
    } else if (typeid(*v) == typeid(BoolVal)) {
        res = new IntVal(((BoolVal*) v)->get() ? 1 : 0);
    }
    
    // Garbage collection
    v->rem_ref();

    return res;
}

Val MapExp::evaluate(Env env) { 
    Val f = func->evaluate(env);
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
    
    Val vs = list->evaluate(env);
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

    } else {
        throw_err("type", "value " + v->toString() + " does not evaluate to a numerical type");
        return NULL;
    }
}
Val NormExp::evaluate(Env env) {
    Val val = exp->evaluate(env);
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


Val NotExp::evaluate(Env env) {
    Val v = exp->evaluate(env);
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

Val OperatorExp::evaluate(Env env) {
    Val a = left->evaluate(env);
    a = unpack_thunk(a);

    if (!a) return NULL;

    Val b = right->evaluate(env);
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

Val PrintExp::evaluate(Env env) {
    string s = "";
    
    for (int i = 0; args[i]; i++) {
        Val v = args[i]->evaluate(env);
        if (!v)
            return NULL;

        if (i) s += " ";
        s += v->toString();
        
        v->rem_ref();
    }

    std::cout << s << "\n";

    return new VoidVal;
}

Val RealExp::evaluate(Env env) {
    return new RealVal(val);
}

Val SequenceExp::evaluate(Env env) {
    Val v = NULL;
    
    // For each expression in the sequence...
    auto it = seq->iterator();
    do {
        // Compute it and store it.
        if (v) v->rem_ref();
        v = it->next()->evaluate(env);
    } while (it->hasNext() && v); // End the loop early if an error occurs.
    
    // Return the final outcome, or NULL if one of
    // the executions fails.
    delete it;
    return v;
}

Val SetExp::evaluate(Env env) {
    Val v = NULL;

    if (typeid(*tgt) == typeid(ListAccessExp)) {
        ListAccessExp *acc = (ListAccessExp*) tgt;
        
        Val u = acc->getList()->evaluate(env);
        if (!u) {
            return NULL;
        } else if (typeid(*u) == typeid(ListVal)) { 

            ListVal *lst = (ListVal*) u;

            Val index = acc->getIdx()->evaluate(env);
            if (!index) {
                u->rem_ref();
                return NULL;
            } else if (typeid(*index) != typeid(IntVal)) {
                throw_type_err(acc->getIdx(), "integer");
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
            
            if (!(v = exp->evaluate(env))) {
                u->rem_ref();
                return NULL;
            } else {
                lst->get()->remove(idx)->rem_ref();
                lst->get()->add(idx, v);
                lst->rem_ref();
            }
        
        } else {
            throw_err("type", "cannot perform list assignment to non-list");
            u->rem_ref();
            return NULL;
        }

    } else if (typeid(*tgt) == typeid(DictAccessExp)) {
        auto acc = (DictAccessExp*) tgt;

        Val u = acc->getList()->evaluate(env);
        if (!u)
            return NULL;
        else if (typeid(*u) == typeid(DictVal)) {
            
            auto lst = (DictVal*) u;

            auto idx = acc->getIdx();

            auto vals = lst->getVals();
            auto vt = vals->iterator();

            bool done = false;
            if (!(v = exp->evaluate(env))) {
                u->rem_ref();
                return NULL;
            }
            
            for (int i = 0; !done && vt->hasNext(); i++) {
                string k = vt->next();
                if (k == idx) {
                    // Reassign
                    vals->remove(k)->rem_ref();
                    vals->add(k, v);
                    done = true;
                }
            }

            delete vt;
            u->rem_ref();
            
            // On failure, new element
            if (!done)
                vals->add(idx, v);
        } else {
            throw_err("type", "cannot perform dictionary assignment to non-dictionary");
            u->rem_ref();
            return NULL;
        }
    
    } else if (typeid(*tgt) == typeid(VarExp)) {
        VarExp *var = (VarExp*) tgt;
        
        v = exp->evaluate(env);
        if (!v) return NULL;
        
        env->set(var->toString(), v);

    } else if (WERROR()) {
        throw_err("runtime", "assigning to right-handish expression '" + tgt->toString() + "' is unsafe");
        return NULL;
    } else {
        throw_warning("runtime", "assigning to right-handish expression '" + tgt->toString() + "' is unsafe");
        // Get info for modifying the environment
        Val u = tgt->evaluate(env);
        if (!u)
            // The variable doesn't exist
            return NULL;

        // Evaluate the expression
        v = exp->evaluate(env);

        if (!v)
            // The value could not be evaluated.
            return NULL;
        
        // Set the new value
        if (u->set(v)) {
            throw_err("runtime", "assignment to right-handish expression '" + tgt->toString() + "' failed");
            v->rem_ref();
            return NULL;
        }
    }
    
    v->add_ref();
    return v;
}

Val StdMathExp::evaluate(Env env) {
    Val v = e->evaluate(env);
    if (!v) return NULL;

    bool isnum = val_is_number(v);

    switch (fn) {
        case SIN:
            if (isnum) {
                auto z = typeid(*v) == typeid(IntVal)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                return new RealVal(sin(z));
            } else {
                throw_err("type", "sin is undefined for inputs outside of R");
                return NULL;
            }
        case COS:
            if (isnum) {
                auto z = typeid(*v) == typeid(IntVal)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                return new RealVal(cos(z));
            } else {
                throw_err("type", "cos is undefined for inputs outside of R");
                return NULL;
            }
        case EXP:
            return exp(v);
        case LOG:
            return log(v);
        case SQRT:
            // sqrt is x^0.5
            Val p = new RealVal(0.5);
            Val y = pow(v, p);
            p->rem_ref();
            return y;
    }

    // In case I do not add it
    throw_err("lomda", "the given math function is undefined");
    return NULL;

}

Val TrueExp::evaluate(Env env) { return new BoolVal(true); }

Val TupleExp::evaluate(Env env) {
    Val l = left->evaluate(env);
    if (!l) return NULL;

    Val r = right->evaluate(env);
    if (!r) { l->rem_ref(); return NULL; }

    return new TupleVal(l, r);
}

Val TupleAccessExp::evaluate(Env env) {
    Val val = exp->evaluate(env);

    if (!val) return NULL;
    else if (typeid(*val) != typeid(TupleVal)) {
        throw_type_err(exp, "tuple");
        val->rem_ref();
        return NULL;
    }
    
    TupleVal *tup = (TupleVal*) val;

    Val Y = idx ? tup->getRight() : tup->getLeft();
    
    // Memory mgt
    Y->add_ref();
    tup->rem_ref();

    return Y;
}

Val VarExp::evaluate(Env env) {
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

Val WhileExp::evaluate(Env env) {
    bool skip = alwaysEnter;

    while (true) {
        Val c = cond->evaluate(env);
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
            Val v = body->evaluate(env);
            if (!v)
                return NULL;
            else
                v->rem_ref();
        } else
            return new VoidVal;
    }
}

