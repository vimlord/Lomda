#include "expression.hpp"
#include "environment.hpp"

#include "config.hpp"

using namespace std;

void throw_calc_err(Exp exp) {
    throw_err("calculus", "expression '" + exp->toString() + "' is non-differentiable\n");
}

Exp reexpress(Val v) {
    if (!v) return NULL;
    else if (typeid(*v) == typeid(IntVal))
        return new IntExp(((IntVal*) v)->get());
    else if (typeid(*v) == typeid(RealVal))
        return new RealExp(((RealVal*) v)->get());
    else if (typeid(*v) == typeid(BoolVal))
        return ((BoolVal*) v)->get() 
                ? (Exp) new TrueExp
                : (Exp) new FalseExp;
    else if (typeid(*v) == typeid(ListVal)) {
        ListVal *lv = (ListVal*) v;
        LinkedList<Exp> *list = new LinkedList<Exp>;

        auto it = lv->get()->iterator();
        while (it->hasNext()) list->add(list->size(), reexpress(it->next()));

        return new ListExp(list);
    } else if (typeid(*v) == typeid(LambdaVal)) {
        LambdaVal *lv = (LambdaVal*) v;

        int argc = 0;
        while (lv->getArgs()[argc] != "");

        std::string *xs = new std::string[argc+1];
        xs[argc] = "";
        while (argc--) xs[argc] = lv->getArgs()[argc];

        return new LambdaExp(xs, lv->getBody());
    } else
        return NULL;
}

Val AndExp::derivativeOf(string x, Env env, Env denv) {
    throw_calc_err(this);
    return NULL;
}

Val ApplyExp::derivativeOf(string x, Env env, Env denv) {
    // d/dx f(u1(x), u2(x), ...) = df/du1 * du1/dx + df/du2 * du2/dx + ...

    Val o = op->valueOf(env);
    if (!o) return NULL;
    else if (typeid(*o) != typeid(LambdaVal)) {
        throw_type_err(op, "lambda");
        return NULL;
    }

    int argc;
    for (argc = 0; args[argc]; argc++);

    // We will utilize the function to perform the calculations
    LambdaVal *func = (LambdaVal*) o;
    
    // Compute the derivative. init cond: d/dx f = 0
    Exp deriv = new IntExp;
    
    for (int i = 0; args[i]; i++) {

        // For the purposes of garbage collection, we will duplicate the
        // functional arguments.
        Exp *xs = new Exp[argc+1];
        xs[argc] = NULL;
        for (int j = 0; j < argc; j++) {
            xs[j] = args[j]->clone();
        }

        // We must apply the ith argument to the derivative
        Exp comp = new MultExp(
            new ApplyExp(
                new DerivativeExp(op->clone(), func->getArgs()[i]),
                xs
            ),
            new DerivativeExp(args[i]->clone(), x)
        );

        if (i) deriv = new SumExp(deriv, comp);
        else {
            delete deriv;
            deriv = comp;
        }
    }
    
    Val v = deriv->valueOf(env);

    o->rem_ref();

    delete deriv;

    return v;
}

Val CompareExp::derivativeOf(string, Env, Env) {
    throw_calc_err(this);
    return NULL;
}

Val DiffExp::derivativeOf(string x, Env env, Env denv) {
    Val a = left->derivativeOf(x, env, denv);
    if (!a) return NULL;

    Val b = right->derivativeOf(x, env, denv);
    if (!b) {
        a->rem_ref();
        return NULL;
    }

    Val c = op(a, b);
    
    a->rem_ref(); b->rem_ref();
    return c;
}

Val DivExp::derivativeOf(string x, Env env, Env denv) { 
    
    Val dl = left->derivativeOf(x, env, denv);
    if (!dl) return NULL;

    Val dr = left->derivativeOf(x, env, denv);
    if (!dr) { dl->rem_ref(); return NULL; }

    Exp a = reexpress(dl);
    Exp b = reexpress(dr);

    // d/dx a/b = (ba' - ab') / (b^2)
    Exp exp = new DivExp(
                        new DiffExp(
                            new MultExp(right->clone(), a),
                            new MultExp(left->clone(), b)),
                        new MultExp(right->clone(), right->clone()));
    Val c = exp->valueOf(env);

    delete exp;
    return c;
}

Val ForExp::derivativeOf(string x, Env env, Env denv) {
    // Evaluate the list
    Val listExp = set->valueOf(env);
    if (!listExp) return NULL;
    else if (typeid(*listExp) != typeid(ListVal)) {
        throw_type_err(set, "list");
        return NULL;
    }
    // And its derivative
    Val dlistExp = set->derivativeOf(x, env, denv);
    if (!dlistExp) return NULL;

    List<Val> *list = ((ListVal*) listExp)->get();
    List<Val> *dlist = ((ListVal*) dlistExp)->get();
    
    // Gather an iterator
    auto it = list->iterator();
    auto dit = dlist->iterator();

    // Value to be return
    Val v = new VoidVal;

    while (it->hasNext()) {
        // Get the next item from the list
        Val v = it->next();
        Val dv = dit->next();
        
        // Build an environment
        Env e = new ExtendEnv(id, v, env);
        Env de = new ExtendEnv(id, dv, denv);
        
        v = body->derivativeOf(x, e, de);

        if (!v) {
            delete it, dit;
            return NULL;
        }

    }

    delete it, dit;

    return v;

}

Val IfExp::derivativeOf(string x, Env env, Env denv) {
    Val b = cond->valueOf(env);

    if (typeid(*b) != typeid(BoolVal)) {
        return NULL;
    }

    BoolVal *B = (BoolVal*) b;
    bool bRes = B->get();

    return (bRes ? tExp : fExp)->derivativeOf(x, env, denv);
}

Val LambdaExp::derivativeOf(string x, Env env, Env denv) {
    int argc;
    for (argc = 0; xs[argc] != ""; argc++);

    string *ids = new string[argc+1];
    ids[argc] = "";
    while (argc--)
        ids[argc] = xs[argc];

    return new LambdaVal(ids, new DerivativeExp(exp->clone(), x), env->clone());
}

Val LetExp::derivativeOf(string x, Env env, Env denv) {
    int argc = 0;
    for (; exps[argc]; argc++);

    // I want to make all of the lambdas recursive.
    // So, I will track my lambdas for now
    LinkedList<LambdaVal*> lambdas;
    
    // Extend the environment
    for (int i = 0; i < argc; i++) {
        // Compute the expression
        Val v = exps[i]->valueOf(env);
        Val dv = exps[i]->derivativeOf(x, env, denv);

        if (!v || !dv) {
            // Garbage collection will happen here
            while (i--) {
                env = env->subenvironment();
                denv = denv->subenvironment();
            }
            return NULL;
        }
        
        // Add it to the environment
        env = new ExtendEnv(ids[i], v->clone(), env);
        
        // Now, we will add the derivative to the environment
        denv = new ExtendEnv(ids[i], dv, denv);

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
    Val y = body->derivativeOf(x, env, denv);
    
    // Garbage collection
    // Not implemented yet
        
    // Return the result
    return y;


}

Val ListExp::derivativeOf(string x, Env env, Env denv) {
    // d/dx [u_0, u_1, ...] = [d/dx u_0, d/dx u_1, ...]
    ListVal *val = new ListVal();
    
    // Add each item
    auto it = list->iterator();
    for(int i = 0; it->hasNext(); i++) {
        // Compute the value of each item
        Exp exp = it->next();
        Val v = exp->derivativeOf(x, env, denv);
        if (!v) {
            delete it;
            return NULL;
        } else
            val->get()->add(i, v);
    }

    delete it;
    return val;
}

Val ListAccessExp::derivativeOf(string x, Env env, Env denv) {
    Val lst = list->derivativeOf(x, env, denv);
    if (!lst) return NULL;
    else if (typeid(*lst) != typeid(ListVal)) {
        throw_type_err(list, "list");
        lst->rem_ref();
        return NULL;
    }

    Val index = idx->valueOf(env);
    if (!index) return NULL;
    else if (typeid(*index) != typeid(IntVal)) {
        throw_type_err(idx, "integer");
        lst->rem_ref();
        index->rem_ref();
        return NULL;
    }

    Val v = ((ListVal*) lst)->get()->get(((IntVal*) index)->get());

    v->add_ref();
    index->rem_ref();
    lst->rem_ref();

    return v;
}

Val MagnitudeExp::derivativeOf(string x, Env env, Env denv) {
    Val res = NULL;
    
    Val v = exp->valueOf(env);

    if (!v) return NULL;
    else if (typeid(*v) == typeid(IntVal) || typeid(*v) == typeid(RealVal)) {
        auto val = (typeid(*v) == typeid(IntVal))
                ? ((IntVal*) v)->get()
                : ((RealVal*) v)->get();

        v->rem_ref();

        Val dv = exp->derivativeOf(x, env, denv);
        if (dv) {
            if (typeid(*dv) == typeid(IntVal))
                res = new IntVal((val >= 0 ? 1 : -1) * ((IntVal*) dv)->get());
            else if (typeid(*dv) == typeid(RealVal))
                res = new IntVal((val >= 0 ? 1 : -1) * ((IntVal*) dv)->get());
            else
                throw_err("runtime", "expression '" + v->toString() + "' does not differentiate to numerical type");
        }

        dv->rem_ref();
    } else {
        throw_err("runtime", "expression '" + v->toString() + "' is not of numerical type");
        v->rem_ref();
    }

    return res;
}

Val MapExp::derivativeOf(string x, Env env, Env denv) {
    Val vs = list->valueOf(env);
    if (!vs) return NULL;

    Val dvs = list->derivativeOf(x, env, denv);
    if (!dvs) {
        // The list could not be differentiated
        vs->rem_ref();
        return NULL;
    }
    
    Val f = func->valueOf(env);
    if (!f) {
        vs->rem_ref();
        dvs->rem_ref();
        return NULL;
    }
    
    if (typeid(*f) != typeid(LambdaVal)) {
        throw_type_err(func, "lambda");
        vs->rem_ref();
        dvs->rem_ref();
        f->rem_ref();
        return NULL;
    }

    // Extract the function... require that it have one argument
    LambdaVal *fn = (LambdaVal*) f;
    if (fn->getArgs()[0] == "" || fn->getArgs()[1] != "") {
        throw_err("runtime", "map function '" + fn->toString() + "' does not take exactly one argument");
        fn->rem_ref();
        vs->rem_ref();
        dvs->rem_ref();
        return NULL;
    }
    
    string var = fn->getArgs()[0];
    fn->rem_ref();
    f = func->derivativeOf(var, env, denv);
    if (!f) {
        vs->rem_ref();
        dvs->rem_ref();
        return NULL;
    } else fn = (LambdaVal*) f;

    if (typeid(*vs) == typeid(ListVal)) {
        // Given a list, map each element of the list
        ListVal *vals = (ListVal*) vs;
        ListVal *dvals = (ListVal*) dvs;

        ListVal *res = new ListVal;
        
        auto it = vals->get()->iterator();
        auto dit = dvals->get()->iterator();
        while (it->hasNext()) {
            Val v = it->next();
            Val dv = dit->next();

            Val *xs = new Val[2];
            xs[0] = v;
            xs[1] = NULL;

            Val elem = fn->apply(xs);

            if (!elem) {
                fn->rem_ref();
                vals->rem_ref();
                dvals->rem_ref();
                res->rem_ref();
                return NULL;
            } else {
                // Compute the other part
                Exp cell = new MultExp(
                    reexpress(elem), reexpress(dv)
                );
                elem->rem_ref();

                elem = cell->valueOf(env);
                delete cell;

                if (elem)
                    // Compute the answer
                    res->get()->add(res->get()->size(), elem);
                else {
                    // The product could not be computed; collect garbage and quit
                    fn->rem_ref();
                    vals->rem_ref();
                    dvals->rem_ref();
                    res->rem_ref();
                    return NULL;
                }
            }
        }
        delete it;
        delete dit;
        
        // Garbage collection
        fn->rem_ref();
        vals->rem_ref();
        dvals->rem_ref();

        return res;

    } else if (typeid(*vs) == typeid(MatrixVal)) {
        Matrix v = ((MatrixVal*) vs)->get();
        Matrix dv = ((MatrixVal*) dvs)->get();
        Matrix M(v.R, v.C);

        Val xs[2];
        xs[1] = NULL;
        
        for (int r = 0; r < M.R; r++)
        for (int c = 0; c < M.C; c++) {
            xs[0] = new RealVal(v(r, c));

            Val elem = fn->apply(xs);
            
            // Garbage collection
            xs[0]->rem_ref();

            if (!elem) {
                // The matrix is non-computable, so failure!
                fn->rem_ref();
                vs->rem_ref();
                dvs->rem_ref();
                return NULL;
            } else if (typeid(*elem) == typeid(RealVal)) {
                M(r, c) = dv(r, c) * ((RealVal*) elem)->get();
            } else if (typeid(*elem) == typeid(IntVal)) {
                M(r, c) = dv(r, c) * (float) (((IntVal*) elem)->get());
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
        throw_type_err(list, "list");
        vs->rem_ref();
        dvs->rem_ref();
        fn->rem_ref();
        return NULL;
    } else {
        throw_warning("runtime", "expression '" + list->toString() + "' does not evaluate as list");

        Val xs[2];
        xs[0] = vs;
        xs[1] = NULL;

        Val v = fn->apply(xs);

        if (v) {
            Exp cell = new MultExp(
                reexpress(v),
                reexpress(dvs)
            );
            v->rem_ref();

            v = cell->valueOf(env);
            delete cell;
        }

        vs->rem_ref();
        dvs->rem_ref();
        fn->rem_ref();

        return v;
    }
}


Val MatrixExp::derivativeOf(string x, Env env, Env denv) {
    Val v = list->derivativeOf(x, env, denv);

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

    List<Val> *arr = array->get();
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
        
        List<Val> *row = ((ListVal*) v)->get();
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
        List<Val> *row = rowval->get();

        auto jt = row->iterator();
        for (int j = 0; j < C; j++, k++) {
            Val f = jt->next();

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

Val MultExp::derivativeOf(string x, Env env, Env denv) { 

    Val dl = left->derivativeOf(x, env, denv);
    if (!dl) return NULL;

    Val dr = left->derivativeOf(x, env, denv);
    if (!dr) { dl->rem_ref(); return NULL; }

    Exp a = reexpress(dl);
    Exp b = reexpress(dr);

    Exp exp = new SumExp(new MultExp(left->clone(), b), new MultExp(right->clone(), a));
    Val c = exp->valueOf(env);

    delete exp;
    return c;

}

Val SetExp::derivativeOf(string x, Env env, Env denv) {
    Val v = NULL;
    Val dv = NULL;

    for (int i = 0; exps[i]; i++) {
        // Get info for modifying the environment
        Val u = tgts[i]->valueOf(env);
        Val du = tgts[i]->derivativeOf(x, env, denv);
        if (!u)
            // The variable doesn't exist
            return NULL;

        // Evaluate the expression
        v = exps[i]->valueOf(env);

        dv = exps[i]->derivativeOf(x, env, denv);

        if (!v || !dv)
            // The value could not be evaluated.
            return NULL;
        
        // Set the new value
        if (u->set(v) | du->set(dv))
            return NULL;

    }
    
    // To be simple, we return 0 on completion
    // as opposed to NULL, which indicates a failure.
    return v ? v : new VoidVal;
}

Val SumExp::derivativeOf(string x, Env env, Env denv) {
    Val a = left->derivativeOf(x, env, denv);
    if (!a) return NULL;

    Val b = right->derivativeOf(x, env, denv);
    if (!b) {
        a->rem_ref();
        return NULL;
    }

    Val c = op(a, b);
    
    a->rem_ref(); b->rem_ref();
    return c;
}

Val VarExp::derivativeOf(string x, Env env, Env denv) {
    // We have a list of derivatives, from which we can easily perform
    // a lookup.
    Val dv = denv->apply(id);
    if (!dv)
        throw_err("calculus", "derivative of variable '" + id + "' is not known within this context");
    else
        dv->add_ref();

    return dv;
}

Val WhileExp::derivativeOf(string x, Env env, Env denv) {
    bool skip = alwaysEnter;
    Val v = new VoidVal;

    while (true) {
        Val c = cond->valueOf(env);
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
            if (!(v = body->derivativeOf(x, env, denv)))
                return NULL;
        } else
            return v;
    }
}


