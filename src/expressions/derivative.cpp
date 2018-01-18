#include "expression.hpp"
#include "environment.hpp"
#include "expressions/derivative.hpp"

#include "config.hpp"

#include <cstdlib>
#include <cmath>

using namespace std;

void throw_calc_err(Exp exp) {
    throw_err("calculus", "expression '" + exp->toString() + "' is non-differentiable\n");
}

void resolveIdentity(Val val, List<int> *idx = NULL) {
    if (!val) return;

    if (typeid(*val) == typeid(ListVal)) {
        if (!idx) idx = new LinkedList<int>;

        ListVal *lst = (ListVal*) val;
        
        auto it = lst->get()->iterator();
        for (int i = 0; it->hasNext(); i++) {
            idx->add(0, i);
            resolveIdentity(it->next(), idx);
            idx->remove(0);
        }

    }

    auto it = idx->iterator();
    auto jt = idx->iterator();
    
    int i;
    for (i = 0; i < idx->size() / 2; i++) jt->next();
    for (; i < idx->size() && it->next() == jt->next(); i++);

    if (i == idx->size()) {
        Val v = typeid(*val) == typeid(IntVal)
                ? (Val) new IntVal(1) : (Val) new RealVal(1);

        val->set(v);
    }
}

Val deriveConstVal(Val v, int c) {

    if (typeid(*v) == typeid(StringVal) ||
        typeid(*v) == typeid(BoolVal))
        // Certain types are non-differentiable
        return NULL;
    else if (typeid(*v) == typeid(ListVal)) {
        auto it = ((ListVal*) v)->get()->iterator();
        LinkedList<Val> *lst = new LinkedList<Val>;
        Val res = new ListVal(lst);

        while (it->hasNext()) {
            Val x = deriveConstVal(it->next(), c);

            if (!x) {
                res->rem_ref();
                delete it;
                return NULL;
            }

            lst->add(lst->size(), x);
        }
        delete it;
        return res; 
    } else
        return new IntVal(c);
}

Val deriveConstVal(Val x, Val v, int c) {

    //return deriveConstVal(x, c);

    if (typeid(*v) == typeid(StringVal) ||
        typeid(*v) == typeid(BoolVal))
        // Certain types are non-differentiable
        return NULL;
    else if (typeid(*v) == typeid(ListVal)) {
        auto it = ((ListVal*) v)->get()->iterator();
        LinkedList<Val> *lst = new LinkedList<Val>;
        Val res = new ListVal(lst);

        while (it->hasNext()) {
            Val u = it->next();
            Val dx = deriveConstVal(x, u, 0);

            if (!dx) {
                res->rem_ref();
                delete it;
                return NULL;
            }

            lst->add(lst->size(), dx);
        }
        delete it;

        if (c == 1) resolveIdentity(res);

        return res;
    } else
        return deriveConstVal(x, c);
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

    Val dr = right->derivativeOf(x, env, denv);
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

Val FoldExp::derivativeOf(string x, Env env, Env denv) {
    // f(L, c) = g(g(...g(c,L[0]),L[1],...))
    // g'(a,b) = a' * g_a(a,b) + b' * g_b(a,b)
    // f' = g'(g(...(c, L[0]), L[1]), ...), L[N-1])
    //      = g'(g(...(c, L[0]),...), L[N-2]) * g_a(...(c,L[0])...,L[N-1])
    //      + L'[N-1] * g_b(...(c,L[0])...,L[N-1])
    Val lst = list->valueOf(env);
    if (!lst) return NULL;
    else if (typeid(*lst) != typeid(ListVal)) {
        lst->rem_ref();
        throw_type_err(list, "list");
        return NULL;
    } else if (((ListVal*) lst)->get()->isEmpty()) {
        // Base case: empty list
        lst->rem_ref();
        return base->derivativeOf(x, env, denv);
    }
    
    Val dlst = list->derivativeOf(x, env, denv);
    if (!dlst) return NULL;
    
    // Attempt to evaluate the fold function
    Val f = func->valueOf(env);
    if (!f) return NULL;
    else if (typeid(*f) != typeid(LambdaVal)) {
        throw_type_err(func, "lambda");

        f->rem_ref();
        lst->rem_ref();
        dlst->rem_ref();

        return NULL;
    }
        
    // Extract the fold function
    LambdaVal *fn = (LambdaVal*) f;
    int i;
    for (i = 0; fn->getArgs()[i] != ""; i++);
    if (i != 2) {
        throw_err("runtime", "function defined by '" + func->toString() + "' does not take exactly two arguments");
        fn->rem_ref();
        lst->rem_ref();
        dlst->rem_ref();
        return NULL;
    }

    // Fold derivative wrt the first var
    Val v = func->derivativeOf(fn->getArgs()[0], env, denv);
    if (!v) {
        fn->rem_ref();
        lst->rem_ref();
        dlst->rem_ref(); 
    }
    LambdaVal *df_a = (LambdaVal*) v;

    // The derivative wrt the second var
    v = func->derivativeOf(fn->getArgs()[1], env, denv);
    if (!v) {
        fn->rem_ref();
        lst->rem_ref();
        dlst->rem_ref(); 
        df_a->rem_ref();
    }
    LambdaVal *df_b = (LambdaVal*) v;

    auto it = ((ListVal*) lst)->get()->iterator();
    auto dit = ((ListVal*) dlst)->get()->iterator();

    Val xs[3];
    xs[2] = NULL;
    
    Val c = base->valueOf(env);
    Val dc = base->derivativeOf(x, env, denv);

    while (c && dc && it->hasNext()) {
        v = it->next();

        xs[0] = c;
        xs[1] = v;
        
        // Perform the fold (and then do GC)
        v = fn->apply(xs);
        
        // Same on derivatives
        Val fa = df_a->apply(xs);
        
        if (!fa) {
            c->rem_ref(); v->rem_ref(); fn->rem_ref(); lst->rem_ref();
            dlst->rem_ref(); df_a->rem_ref(); df_b->rem_ref();
            return NULL;
        }

        Val fb = df_b->apply(xs);
        if (!fb) {
            fa->rem_ref();
            c->rem_ref(); v->rem_ref(); fn->rem_ref(); lst->rem_ref();
            dlst->rem_ref(); df_a->rem_ref(); df_b->rem_ref();
            return NULL;
        }

        // Apply the fold
        c->rem_ref();
        c = v;

        // Derivative of the list index
        v = dit->next();
        
        // We will construct an expression to handle the ordeal
        Expression *cell = new SumExp(
            new MultExp(reexpress(dc), reexpress(fa)),
            new MultExp(reexpress(v), reexpress(fb))
        );

        v = cell->valueOf(env);

        // Large amount of GC
        dc->rem_ref();
        fa->rem_ref();
        fb->rem_ref();

        dc = v;

        delete cell;
    }
    
    // Metric fuckton of GC
    if (c) c->rem_ref();
    fn->rem_ref();
    lst->rem_ref();
    dlst->rem_ref();
    df_a->rem_ref();
    df_b->rem_ref();

    return dc;

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

Val MultExp::derivativeOf(string x, Env env, Env denv) { 
    // Left hand derivative
    Val dl = left->derivativeOf(x, env, denv);
    if (!dl) return NULL;
    
    // Right hand derivative
    Val dr = right->derivativeOf(x, env, denv);
    if (!dr) { dl->rem_ref(); return NULL; }
    
    // We will need the left and right sides
    Val l = left->valueOf(env);
    if (!l) { dl->rem_ref(); dr->rem_ref(); return NULL; }
    Val r = right->valueOf(env);
    if (!r) { dl->rem_ref(); dr->rem_ref(); l->rem_ref(); return NULL; }
    
    // Utility object
    SumExp sum(NULL, NULL);

    Val a = op(l, dr);
    l->rem_ref();
    dr->rem_ref();

    if (!a) {
        r->rem_ref();
        dl->rem_ref();
        return NULL;
    }

    Val b = op(r, dl);
    r->rem_ref();
    dl->rem_ref();

    if (!b) {
        a->rem_ref();
        return NULL;
    }
    
    Val c = sum.op(a, b);

    a->rem_ref();
    b->rem_ref();

    return c;
}

Val SetExp::derivativeOf(string x, Env env, Env denv) {
    Val v = NULL;
    Val dv = NULL;

    if (typeid(*exp) == typeid(ListAccessExp)) {
        ListAccessExp *acc = (ListAccessExp*) tgt;
        
        Val u = acc->getList()->valueOf(env);
        Val du;

        if (!u) {
            return NULL;
        } else if (typeid(*u) != typeid(ListVal)) {
            u->rem_ref();
            return NULL;
        } else if (!(du = acc->getList()->derivativeOf(x, env, denv))) {
            u->rem_ref();
            return NULL;
        }

        ListVal *lst = (ListVal*) u;
        ListVal *dlst = (ListVal*) du;

        Val index = acc->getIdx()->valueOf(env);
        if (!index) {
            u->rem_ref();
            du->rem_ref();
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
            du->rem_ref();
            return NULL;
        }
        
        if (!(v = exp->valueOf(env))) {
            u->rem_ref();
            du->rem_ref();
            return NULL;
        } else if (!(dv = exp->derivativeOf(x, env, denv))) {
            u->rem_ref();
            du->rem_ref();
            v->rem_ref();
            return NULL;
        } else {
            lst->get()->remove(idx)->rem_ref();
            lst->get()->add(idx, v);
            dlst->get()->remove(idx)->rem_ref();
            dlst->get()->add(idx, dv);
        }

    } else if (typeid(*exp) == typeid(VarExp)) {
        VarExp *var = (VarExp*) tgt;
        
        v = exp->valueOf(env);
        if (!v) return NULL;
        dv = exp->derivativeOf(x, env, denv);
        if (!dv) { v->rem_ref(); return NULL; }
        
        env->set(var->toString(), v);
        denv->set(var->toString(), dv);

    } else {
        // Get info for modifying the environment
        Val u = tgt->valueOf(env);
        if (!u)
            // The variable doesn't exist
            return NULL;
        Val du = tgt->derivativeOf(x, env, denv);
        if (!du) {
            u->rem_ref();
            du->rem_ref();
        }

        // Evaluate the expression
        v = exp->valueOf(env);

        if (!v) {
            u->rem_ref();
            du->rem_ref();

            // The value could not be evaluated.
            return NULL;
        }

        // Evaluate the expression
        dv = exp->derivativeOf(x, env, denv);

        if (!dv) {
            u->rem_ref();
            du->rem_ref();
            v->rem_ref();
            return NULL;
        }
        
        // Set the new value
        if (u->set(v)) {
            u->rem_ref();
            du->rem_ref();
            v->rem_ref();
            dv->rem_ref();
            return NULL;
        } else if (du->set(dv)) {
            u->rem_ref();
            du->rem_ref();
            v->rem_ref();
            dv->rem_ref();
            return NULL;
        }
    }
    
    dv->add_ref();
    return dv;
}

Val StdlibOpExp::derivativeOf(string id, Env env, Env denv) {
    Val v = x->valueOf(env);
    if (!v) return NULL;
    
    if (
        typeid(*v) == typeid(LambdaVal)
    ||  typeid(*v) == typeid(ListVal)
    ||  typeid(*v) == typeid(StringVal)
    ) {
        throw_type_err(x, "numerical");
    }

    auto z = typeid(*v) == typeid(IntVal)
            ? ((IntVal*) v)->get()
            : ((RealVal*) v)->get();
    v->rem_ref();

    Val dx = x->derivativeOf(id, env, denv);
    if (!dx) {
        v->rem_ref();
        return NULL;
    }

    switch (op) {
        case SIN: v = new RealVal(cos(z)); break; // d/dx sin x = cos x
        case COS: v = new RealVal(-sin(z)); break; // d/dx cos x = -sin x
        case LOG: v = new RealVal(1.0 / z); break; // d/dx ln x = 1/x
        case SQRT: v = new RealVal(1.0 / (2 * sqrt(z))); break; // d/dx sqrt x = 1 / (2 sqrt x)
    }
    
    MultExp mult(NULL, NULL);

    Val y = mult.op(v, dx);
    v->rem_ref();
    dx->rem_ref();

    return y;
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

// d/dx c = 0
Val IntExp::derivativeOf(string x, Env env, Env denv) {
    return deriveConstVal(env->apply(x), 0);
}
Val RealExp::derivativeOf(string x, Env env, Env denv) {
    return deriveConstVal(env->apply(x), 0);
}


