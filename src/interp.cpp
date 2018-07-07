#include "interp.hpp"

#include "expression.hpp"
#include "value.hpp"
#include "baselang/environment.hpp"

#include "parser.hpp"
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
    Exp exp = parse_program(program);

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

        Env env = new Environment;

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

Val UnaryOperatorExp::evaluate(Env env) {
    Val x = exp->evaluate(env);
    if (!x) return NULL;

    Val y = op(x);
    x->rem_ref();

    return y;
}

Val AdtExp::evaluate(Env env) {
    // Determine the argument count.
    int argc;
    for (argc = 0; args[argc]; argc++);
    
    // Build an argument list
    Val *xs = new Val[argc+1];
    xs[argc] = NULL;
    for (int i = 0; i < argc; i++) {
        // Evaluate the argument
        xs[i] = args[i]->evaluate(env);
        if (!xs[i]) {
            // Evaluation failed, hence we must return.
            while (i--)
                xs[i]->rem_ref();
            return NULL;
        }
    }
    
    return new AdtVal(name, kind, xs);
}
Val AdtDeclarationExp::evaluate(Env env) {
    auto subtypes = new Trie<Val>;
    
    auto adt = new DictVal(subtypes);

    for (int i = 0; argss[i]; i++) {
        auto kind = ids[i];
        int j;
        for (j = 0; argss[i][j]; j++);
        
        // Generate a list of arguments for the constructor.
        auto xs = new string[j+1];

        // Generate a list of arguments for the backend factory.
        auto zs = new Exp[j+1];

        xs[j] = "";
        zs[j] = NULL;
        while (j--) {
            xs[j] = "arg" + to_string(j);
            zs[j] = new VarExp(xs[j]);
        }
        
        // Add the declaration
        subtypes->add(kind, new LambdaVal(xs, new AdtExp(name, kind, zs)));
    }
    
    // Store the dictionary. This holds all of the constructors.
    env->set(name, adt);
    adt->rem_ref();

    // Display the env if necessary
    throw_debug("env", "original env Γ extended to env Γ' := " + env->toString());
    
    // Now, we can evaluate.
    Val res = body->evaluate(env);
    
    // Now, we remove the item.
    env->rem(name);

    return res;
}
Val SwitchExp::evaluate(Env env) {
    
    Val val = adt->evaluate(env);
    if (!val) return NULL;
    else if (!isVal<AdtVal>(val)) {
        throw_type_err(adt, "ADT");
        val->rem_ref();
        return NULL;
    }

    auto A = (AdtVal*) val;
    
    // Count the number of arguments
    int xs;
    for (xs = 0; A->getArgs()[xs]; xs++);
    
    // Find the right argument set
    string name = A->getKind();
    string *ids = NULL;
    int i;
    for (i = 0; !ids && idss[i]; i++) {
        if (names[i] == name) {
            // Compute the argument counts
            int zs;
            for (zs = 0; idss[i][zs] != ""; zs++);
            
            // Check if a match was found
            if (xs == zs) {
                ids = idss[i];
                break;
            }
        }
    }
    
    // Check that the ADT was compatible.
    if (!ids) {
        throw_err("type", "adt " + A->toString() + " is incompatible with the switch statement; see:\n\t" + toString());
        val->rem_ref();
        return NULL;
    }
    
    // Grab the correct body
    Exp body = bodies[i];

    // Now, we will extend the environment
    for (i = 0; i < xs; i++)
        env->set(ids[i], A->getArgs()[i]);

    Val res = body->evaluate(env);
    
    // Perform garbage collection
    for (i = 0; i < xs; i++)
        env->rem(ids[i]);
    val->rem_ref();

    return res;
}

Val ApplyExp::evaluate(Env env) {
    Val f = op->evaluate(env);

    // Null check the function
    if (!f)
        return NULL;
    else
        f = unpack_thunk(f);
    
    // Type check the function
    if (!isVal<LambdaVal>(f)) {
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
        if (isVal<IntVal>(val))
            return val;
        else if (isVal<RealVal>(val))
            res = new IntVal(((RealVal*) val)->get());
        else if (isVal<BoolVal>(val))
            res = new IntVal(((BoolVal*) val)->get());
        else if (isVal<StringVal>(val)) {
            // String parsing (use BNF parser source)
            string s = ((StringVal*) val)->get();

            string::size_type Zlen = -1;
            int Z;
            try { Z = stoi(s, &Zlen, 10); }
            catch (std::out_of_range oor) { Zlen = -1; }
            catch (std::invalid_argument ia) { Zlen = -1; }
            
            if ((int) Zlen > 0)
                res = new IntVal(Z);
            else {
                throw_err("cast", "cannot parse int from " + s);
                return NULL;
            }
        }
    } else if (isType<RealType>(type)) {
        // Type conversion to a float
        if (isVal<IntVal>(val))
            res = new RealVal(((IntVal*) val)->get());
        else if (isVal<RealVal>(val))
            return val;
        else if (isVal<BoolVal>(val))
            res = new RealVal(((BoolVal*) val)->get());
        else if (isVal<StringVal>(val)) {
            // String parsing
            string s = ((StringVal*) val)->get();

            string::size_type len = -1;
            int R;
            try { R = stol(s, &len); }
            catch (std::out_of_range oor) { len = -1; }
            catch (std::invalid_argument ia) { len = -1; }
            
            if ((int) len > 0)
                res = new RealVal(R);
            else {
                throw_err("cast", "cannot parse R from " + s);
                return NULL;
            }
        }
    } else if (isType<BoolType>(type)) {
        // Type conversion to a boolean
        if (isVal<IntVal>(val))
            res = new BoolVal(((IntVal*) val)->get());
        else if (isVal<RealVal>(val))
            res = new BoolVal(((RealVal*) val)->get());
        else if (isVal<BoolVal>(val))
            return val;
        else if (isVal<StringVal>(val)) {
            // String parsing
            string s = ((StringVal*) val)->get();
            
            int i;
            for (i = 0; s[i] == ' ' || s[i] == '\n' || s[i] == '\t'; i++);

            if (s.substr(i, 4) == "true"
                    && s[4] != '_' && !(s[4] >= 'a' && s[4] <= 'z') && !(s[4] >= 'A' && s[4] <= 'Z')) {
                res = new BoolVal(true);
            } else if (s.substr(i, 5) == "false"
                    && s[5] != '_' && !(s[5] >= 'a' && s[5] <= 'z') && !(s[5] >= 'A' && s[5] <= 'Z')) {
                res = new BoolVal(false);
            } else {
                throw_err("cast", "cannot parse bool from " + s);
                return NULL;
            }
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

    Exp symb = func->symb_diff(var);
    if (symb && !isExp<DerivativeExp>(symb)) {
        throw_debug("calc_init", toString() + " = " + symb->toString());
        // Put off the evaluation until a later time.
        Val v = symb->evaluate(env);
        delete symb;
        return v;
    } else
        delete symb;

    if (!env->apply(var)) {
        // The differentiation MUST be over a lambda
        Val v = func->evaluate(env);
        if (!v) return NULL;
        else if (!isVal<LambdaVal>(v)) {
            throw_type_err(func, "lambda");
            v->rem_ref();
            return NULL;
        } else {
            // We can compute the derivative.
            auto f = (LambdaVal*) v;
            
            // Argument count
            int argc;
            for (argc = 0; f->getArgs()[argc] != ""; argc++);

            // Argument list
            string *ids = new string[argc+1];
            ids[argc] = "";
            while (argc--) ids[argc] = f->getArgs()[argc];
            
            // Environment
            env = f->getEnv();
            env->add_ref();
            
            // Body
            auto exp = f->getBody()->symb_diff(var);

            v->rem_ref();
            
            // Build the final result
            return new LambdaVal(ids, exp, env);
        }
    }

    // Base case: we know of nothing
    Env denv = new Environment;
    
    // Compute the base case derivative for each variable.
    for (auto x_v : env->get_store()) {
        string id = x_v.first;
        Val v = x_v.second;
        
        if (isVal<LambdaVal>(v)) {
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
            denv->set(id, dv);
            dv->rem_ref(); // The derivative exists only within the environment
        } else {
            // Trivial derivative: d/dx c = 0, d/dx x = x
            int c = id == var ? 1 : 0;
            
            Val X = env->apply(var);
            v = X ? deriveConstVal(var, v, X, c) : NULL;

            if (v) {
                // The value is of a differentiable type
                denv->set(id, v);
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
            delete kt;
            delete vt;
            delete res;
            return NULL;
        }

        string k = kt->next();

        vs->add(k, v);
    }

    delete kt;
    delete vt;

    return res;

}

Val FalseExp::evaluate(Env env) { return new BoolVal(false); }

Val FoldExp::evaluate(Env env) {
    Val lst = list->evaluate(env);
    lst = unpack_thunk(lst);

    if (!lst) return NULL;
    else if (!isVal<ListVal>(lst)) {
        lst->rem_ref();
        throw_type_err(list, "list");
        return NULL;
    }
    
    Val f = func->evaluate(env);
    f = unpack_thunk(f);

    if (!f) return NULL;
    else if (!isVal<LambdaVal>(f)) {
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

    Val xs[3];
    xs[0] = base->evaluate(env); // First slot is the accumulator
    xs[2] = NULL; // Last slot is a null terminator.

    for (int i = 0; i < ((ListVal*) lst)->size() && xs[0]; i++) {
        // Second slot is the element
        xs[1] = ((ListVal*) lst)->get(i);
        
        // Update the accumulator.
        Val acc = fn->apply(xs);
        xs[0]->rem_ref();
        xs[0] = acc;
    }

    fn->rem_ref();
    lst->rem_ref();

    return xs[0];
}

Val ForExp::evaluate(Env env) {
    // Evaluate the list
    Val listExp = set->evaluate(env);
    listExp = unpack_thunk(listExp);

    if (!listExp) return NULL;
    else if (!isVal<ListVal>(listExp)) {
        throw_type_err(set, "list");
        return NULL;
    }
    List<Val> *list = (ListVal*) listExp;
    
    for (int i = 0; i < list->size(); i++) {
        // Get the next item from the list
        Val x = list->get(i);
        
        // Temporarily store the old value
        Val tmp = env->apply(id);
        if (tmp) tmp->add_ref();

        // We are going to modify the environment temporarily
        env->set(id, x);
        
        // Under our modified context, we compute the value
        Val v = body->evaluate(env);  

        // Now, we reset. We either replace the value or remove it.
        if (tmp) {
            env->set(id, tmp);
            tmp->rem_ref();
        } else
            env->rem(id);


        if (!v) {
            listExp->rem_ref();
            return NULL;
        } else
            v->rem_ref();
    }

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
        List<Val> *lst = (ListVal*) xs;
        
        for (int i = 0; i < lst->size() && !res; i++) {
            Val v = lst->get(i);
            
            Val b = equals.op(x, v);
            if (!b) {
                delete x;
                delete xs;
                return NULL;
            } else if (((BoolVal*) b)->get())
                res = new BoolVal(true);
        }
        if (!res) res = new BoolVal(false);

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
    else if (!isVal<BoolVal>(b)) {
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
    Val mod;
    if (module_cache.find(module) != module_cache.end()) {
        // The module is currently stored in the cache
        mod = module_cache[module];
        throw_debug("module IO", "loaded " + module + " from cache");
    } else if ((mod = load_stdlib(module))) {
        // The module is a standard library
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

        // Store the module in the cache if need be
        if (USE_MODULE_CACHING() && module_cache.find(module) == module_cache.end()) {
            throw_debug("module", "caching module " + module);
            module_cache[module] = mod;
        }
        
        // We will extend the environment with the newly found module
        Val tmp = env->apply(name);
        if (tmp) tmp->add_ref();

        env->set(name, mod);
        
        // Evaluate the subexpression.
        Val v = exp->evaluate(env);

        // Garbage collection
        env->rem(name);
        if (tmp) {
            env->set(name, tmp);
            tmp->rem_ref();
        }

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
    if (isVal<IntVal>(val))
        return isType<IntType>(type) || isType<RealType>(type);
    else if (isVal<RealVal>(val))
        return isType<RealType>(type) && (!isType<IntType>(type) || isType<VarType>(type));
    else if (isVal<BoolVal>(val))
        return isType<BoolType>(type);
    else if (isVal<StringVal>(val))
        return isType<StringType>(type);
    else if (isVal<TupleVal>(val)) {
        // Type each side if it is a tuple.
        return isType<TupleType>(type) &&
            static_typecheck(((TupleVal*) val)->getLeft(), ((TupleType*) type)->getLeft())
        &&  static_typecheck(((TupleVal*) val)->getRight(), ((TupleType*) type)->getRight());
    } else if (isVal<ListVal>(val)) {
        if (isType<ListType>(type)) {
            // We must check each element for correctness.
            auto T = ((ListType*) type)->subtype();
            auto lst = ((ListVal*) val);
            bool res = true;
            for (int i = 0; res && i < lst->size(); i++)
                res = static_typecheck(lst->get(i), T);
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

    // Extend the environment
    for (int i = 0; i < argc; i++) {
        // Compute the expression
        Val v = exps[i]->evaluate(env);

        if (!v) {
            // Garbage collect and escape
            while (i--)
                env->rem(ids[i]);
            return NULL;
        }

        // Add it to the environment
        Val x = v->clone();
        env->set(ids[i], x);
        
        // Drop references
        v->rem_ref();
        x->rem_ref();

        // We permit lambdas that request recursion to have it.
        if (rec && rec[i] && isVal<LambdaVal>(x))
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
    while (argc--) {
        env->rem(ids[argc]);
    }
        
    // Return the result
    return y;

}

Val ListExp::evaluate(Env env) {
    // Generate a blank list
    ListVal *val = new ListVal;
    
    // Add each item
    for(int i = 0; i < size(); i++) {
        // Compute the value of each item
        Val v = get(i)->evaluate(env);

        if (!v) {
            // Garbage collection on the iterator and the value
            delete val;

            return NULL;
        } else {
            val->add(i, v);
        }
    }

    return val;
}

Val ListAccessExp::evaluate(Env env) {
    // Get the list
    Val f = list->evaluate(env);
    f = unpack_thunk(f);

    if (!f)
        return NULL;
    else if (!isVal<ListVal>(f)) {
        throw_type_err(list, "list");
        return NULL;
    }
    
    // Get the index
    Val index = idx->evaluate(env);
    index = unpack_thunk(index);

    if (!index) return NULL;  

    // The list
    auto vals = (ListVal*) f;

    if (!isVal<IntVal>(index)) {
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
    else if (!isVal<DictVal>(f)) {
        throw_type_err(list, "dict, list, or string");
        f->rem_ref();
        return NULL;
    } else {
        DictVal *d = (DictVal*) f;
        Val v;

        if (d->getVals()->hasKey(idx)) {
            v = d->getVals()->get(idx);
            v->add_ref();
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
    else if (!isVal<ListVal>(f)) {
        throw_type_err(list, "list");
        return NULL;
    }
    auto vals = (ListVal*) f;

    // Compute the index
    Val index = idx->evaluate(env);
    index = unpack_thunk(index);

    if (!index) return NULL;
    else if (!isVal<IntVal>(index)) {
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
    else if (!isVal<ListVal>(f)) {
        throw_type_err(list, "list");
        return NULL;
    }
    auto vals = (ListVal*) f;

    // Compute the index
    Val index = idx->evaluate(env);
    index = unpack_thunk(index);

    if (!index) return NULL;
    else if (!isVal<IntVal>(index)) {
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
    else if (!isVal<ListVal>(lst)) {
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
        else if (!isVal<IntVal>(f)) {
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
        else if (!isVal<IntVal>(t)) {
            throw_type_err(to, "integer");
            lst->rem_ref(); // Garbage collection
            t->rem_ref();
            return NULL;
        }

        j = ((IntVal*) t)->get();
        t->rem_ref();
    } else if (isVal<ListVal>(lst))
        j = ((ListVal*) lst)->size();
    else
        j = lst->toString().length();
    
    // The list
    auto vals = (ListVal*) lst;

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

Val MagnitudeExp::op(Val v) {
    if (isVal<IntVal>(v)) {
        // Magnitude of number is its absolute value
        int val = ((IntVal*) v)->get();
        return new IntVal(val > 0 ? val : -val);
    } else if (isVal<RealVal>(v)) {
        // Magnitude of number is its absolute value
        float val = ((RealVal*) v)->get();
        return new RealVal(val > 0 ? val : -val);
    } else if (isVal<ListVal>(v)) {
        // Magnitude of list is its length
        int val = ((ListVal*) v)->size();
        return new IntVal(val);
    } else if (isVal<StringVal>(v)) {
        return new IntVal(v->toString().length());
    } else if (isVal<BoolVal>(v)) {
        return new IntVal(((BoolVal*) v)->get() ? 1 : 0);
    } else {
        return NULL;
    }
}

Val MapExp::evaluate(Env env) { 
    Val f = func->evaluate(env);
    f = unpack_thunk(f);
    if (!f) return NULL;
    
    if (!isVal<LambdaVal>(f)) {
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

    if (isVal<ListVal>(vs)) {
        // Given a list, map each element of the list
        ListVal *vals = (ListVal*) vs;
        ListVal *res = new ListVal;
        
        for (int i = 0; i < vals->size(); i++) {
            Val v = vals->get(i);

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
                res->add(i, elem);
            }
        }
        
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

Val sqnorm(Val v) {
    if (isVal<IntVal>(v)) {
        // Magnitude of number is its absolute value
        int val = ((IntVal*) v)->get();
        return new IntVal(val * val);
    } else if (isVal<RealVal>(v)) {
        // Magnitude of number is its absolute value
        int val = ((RealVal*) v)->get();
        return new RealVal(val * val);
    } else if (isVal<ListVal>(v)) {
        // Magnitude of list is its length
        auto it = ((ListVal*) v)->iterator();

        float sum = 0;
        
        while (it->hasNext()) {
            Val v = sqnorm(it->next());

            if (!v) return NULL;

            auto x = isVal<IntVal>(v)
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
Val NormExp::op(Val val) {

    Val v = sqnorm(val);
    if (!v) return NULL;

    auto x = isVal<IntVal>(v)
            ? ((IntVal*) v)->get()
            : ((RealVal*) v)->get();

    v->rem_ref();
    
    return x < 0 ? NULL : new RealVal(sqrt(x));
}


Val NotExp::op(Val v) {
    if (!isVal<BoolVal>(v)) {
        throw_type_err(exp, "boolean");
        return NULL;
    } else {
        BoolVal *B = (BoolVal*) v;
        bool b = B->get();
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

    if (isExp<ListAccessExp>(tgt)) {
        // We are attempting to assign to a list
        ListAccessExp *acc = (ListAccessExp*) tgt;
        
        Val u = acc->getList()->evaluate(env);
        if (!u) {
            return NULL;
        } else if (isVal<ListVal>(u)) { 

            ListVal *lst = (ListVal*) u;

            Val index = acc->getIdx()->evaluate(env);
            if (!index) {
                u->rem_ref();
                return NULL;
            } else if (!isVal<IntVal>(index)) {
                throw_type_err(acc->getIdx(), "integer");
                index->rem_ref();
                u->rem_ref();
                return NULL;
            }

            int idx = ((IntVal*) index)->get();
            index->rem_ref();
            
            if (idx < 0 || lst->size() <= idx) {
                u->rem_ref();
                return NULL;
            }
            
            if (!(v = exp->evaluate(env))) {
                u->rem_ref();
                return NULL;
            } else {
                v->add_ref();

                lst->remove(idx)->rem_ref();
                lst->add(idx, v);
                lst->rem_ref();
            }
        
        } else {
            throw_err("type", "cannot perform list assignment to non-list");
            u->rem_ref();
            return NULL;
        }

    } else if (isExp<DictAccessExp>(tgt)) {
        // We are attempting to assign to a dictionary
        auto acc = (DictAccessExp*) tgt;

        Val u = acc->getList()->evaluate(env);
        if (!u)
            return NULL;
        else if (isVal<DictVal>(u)) {
            
            auto lst = (DictVal*) u;

            auto idx = acc->getIdx();

            auto vals = lst->getVals();
            auto vt = vals->iterator();

            bool done = false;
            if (!(v = exp->evaluate(env))) {
                u->rem_ref();
                return NULL;
            } else
                v->add_ref();
            
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
    
    } else if (isExp<VarExp>(tgt)) {
        // We are attempting to perform direct assignment to a variable
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
        } else
            v->add_ref();
    }
    
    return v;
}

Val StdMathExp::evaluate(Env env) {
    Val v = e->evaluate(env);
    if (!v) return NULL;

    bool isnum = val_is_number(v);
    
    // Is the value a list of numbers
    bool islst = val_is_list(v);
    if (islst) {
        auto it = ((ListVal*) v)->iterator();
        while (islst && it->hasNext())
            islst = val_is_number(it->next());
        delete it;
    }

    Val y = NULL;

    switch (fn) {
        case MIN:
        case MAX:
            if (islst) {
                ListVal *lst = (ListVal*) v;
                // Check on empty lists
                if (lst->size() == 0) {
                    throw_err("runtime", "max is undefined on empty lists");
                    break;
                }

                CompareExp gt(NULL, NULL, GT);
                
                // Iterator and initial condition.
                auto it = lst->iterator();
                y = it->next();

                while (it->hasNext()) {
                    Val val = it->next();

                    BoolVal *b = (BoolVal*) gt.op(val, y);
                    if (b->get() == (fn == MAX))
                        y = val;
                    b->rem_ref();
                }
                delete it;

                y->add_ref();

            } else
                throw_err("type", "max is undefined for inputs outside of [R]");
            break;
        case SIN:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                y = new RealVal(sin(z));
            } else
                throw_err("type", "sin is undefined for inputs outside of R");
            break;
        case COS:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                y = new RealVal(cos(z));
            } else
                throw_err("type", "cos is undefined for inputs outside of R");
            break;
        case TAN:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                y = new RealVal(tan(z));
            } else
                throw_err("type", "tan is undefined for inputs outside of R");
            break;
        case ASIN:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                y = new RealVal(asin(z));
            } else
                throw_err("type", "arcsin is undefined for inputs outside of R");
            break;
        case ACOS:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                y = new RealVal(acos(z));
            } else
                throw_err("type", "arccos is undefined for inputs outside of R");
            break;
        case ATAN:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                y = new RealVal(atan(z));
            } else
                throw_err("type", "arctan is undefined for inputs outside of R");
            break;
        case SINH:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                y = new RealVal(sinh(z));
            } else
                throw_err("type", "sinh is undefined for inputs outside of R");
            break;
        case COSH:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                y = new RealVal(cosh(z));
            } else
                throw_err("type", "cosh is undefined for inputs outside of R");
            break;
        case TANH:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                y = new RealVal(tanh(z));
            } else
                throw_err("type", "tanh is undefined for inputs outside of R");
            break;
        case ASINH:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                y = new RealVal(asinh(z));
            } else
                throw_err("type", "arcsinh is undefined for inputs outside of R");
            break;
        case ACOSH:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                y = new RealVal(acosh(z));
            } else
                throw_err("type", "arccosh is undefined for inputs outside of R");
            break;
        case ATANH:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                y = new RealVal(atanh(z));
            } else
                throw_err("type", "arctanh is undefined for inputs outside of R");
            break;
        case EXP:
            y = exp(v);
            break;
        case LOG:
            y = log(v);
            break;
        case SQRT: {
            // sqrt is x^0.5
            Val p = new RealVal(0.5);
            y = pow(v, p);
            p->rem_ref();
            break;
        } default:
            throw_err("lomda", "the given math function is undefined");
    }
    
    // Garbage collection
    v->rem_ref();

    // In case I do not add it
    return y;

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
    else if (!isVal<TupleVal>(val)) {
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
        Val c = skip ? new BoolVal(true) : cond->evaluate(env);
        c = unpack_thunk(c);

        skip = false; // Do not do do-while from now on.

        if (!isVal<BoolVal>(c)) {
            throw_type_err(cond, "boolean");
            return NULL;
        }
        
        bool cond = ((BoolVal*) c)->get();
        c->rem_ref();
        
        if (cond) {
            // Compute the new outcome. If it is
            // NULL, computation failed, so NULL
            // should be returned.
            Val v = body->evaluate(env);
            if (!v)
                return NULL;
            else
                v->rem_ref();
        } else
            // On success, the return type is void
            return new VoidVal;
    }
}

