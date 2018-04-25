#include "expression.hpp"
#include "environment.hpp"
#include "expressions/derivative.hpp"

#include "config.hpp"
#include "types.hpp"

#include <cstdlib>
#include <cmath>
#include "math.hpp"

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

        if (idx->size() == 0) delete idx;
    } else if (typeid(*val) == typeid(DictVal)) {
        if (!idx) idx = new LinkedList<int>;
        
        DictVal *dct = (DictVal*) val;

        auto vit = dct->getVals()->iterator();
        for (int i = 0; vit->hasNext(); i++) {
            string k = vit->next();
            idx->add(0, i);
            resolveIdentity(dct->getVals()->get(k), idx);
            idx->remove(0);
        }

        if (idx->size() == 0) delete idx;
    } else if (typeid(*val) == typeid(TupleVal)) {
        if (!idx) idx = new LinkedList<int>;

        idx->add(0, 0);
        resolveIdentity(((TupleVal*) val)->getLeft(), idx);
        idx->remove(0);

        idx->add(0, 1);
        resolveIdentity(((TupleVal*) val)->getRight(), idx);
        idx->remove(0);

        if (idx->size() == 0) delete idx;
    } else if (idx->size() % 2 == 0) {

        auto it = idx->iterator();
        auto jt = idx->iterator();
        
        int i;
        for (i = 0; i < idx->size() / 2; i++) jt->next();
        for (; i < idx->size() && it->next() == jt->next(); i++);

        if (i == idx->size()) {
            val->set(typeid(*val) == typeid(IntVal) ? (Val) new IntVal(1) : (Val) new RealVal(1));
        }
    }
}

Val deriveConstVal(string id, Val v, int c) {

    if (typeid(*v) == typeid(StringVal) ||
        typeid(*v) == typeid(BoolVal))
        // Certain types are non-differentiable
        return NULL;
    else if (typeid(*v) == typeid(ListVal)) {
        auto it = ((ListVal*) v)->get()->iterator();

        auto lst = new ArrayList<Val>;
        Val res = new ListVal(lst);

        while (it->hasNext()) {
            Val x = deriveConstVal(id, it->next(), c);

            if (!x) {
                res->rem_ref();
                delete it;
                return NULL;
            }

            lst->add(lst->size(), x);
        }
        delete it;
        return res; 
    } else if (typeid(*v) == typeid(DictVal)) {
        auto vit = ((DictVal*) v)->getVals()->iterator();

        auto vals = new Trie<Val>;
        
        Val res = new DictVal(vals);

        while (vit->hasNext()) {
            auto k = vit->next();
            
            vals->add(k, deriveConstVal(id, ((DictVal*) v)->getVals()->get(k), c));
        }

        delete vit;

        return res;

    } else if (typeid(*v) == typeid(TupleVal)) {
        Val L = deriveConstVal(id, ((TupleVal*) v)->getLeft(), c);
        if (!L) return NULL;

        Val R = deriveConstVal(id, ((TupleVal*) v)->getRight(), c);
        if (!R) { L->rem_ref(); return NULL; }

        L->add_ref();
        R->add_ref();

        return new TupleVal(L, R);

    } else if (typeid(*v) == typeid(LambdaVal)) {
        auto L = (LambdaVal*) v;

        auto body = new DerivativeExp(L->getBody()->clone(), id);

        auto xs = L->getArgs();
        int i;
        for (i = 0; xs[i] != ""; i++);

        auto ids = new string[i+1];
        ids[i] = "";
        while (i--) ids[i] = xs[i];

        auto env = L->getEnv();
        if (env) env->add_ref();
        else env = new EmptyEnv;

        return new LambdaVal(ids, body, env);
    } else if (typeid(*v) == typeid(IntVal) || typeid(*v) == typeid(RealVal))
        return new IntVal(c);
    else
        return NULL;
}

/**
 * Computes dy/dx ~= c
 */
Val deriveConstVal(string id, Val y, Val x, int c) {

    //return deriveConstVal(id, y, c);

    if (typeid(*x) == typeid(StringVal) ||
        typeid(*x) == typeid(BoolVal))
        // Certain types are non-differentiable
        return NULL;
    else if (typeid(*x) == typeid(ListVal)) {
        auto it = ((ListVal*) x)->get()->iterator();
        
        // Resulting derivative
        auto lst = new ArrayList<Val>;
        Val res = new ListVal(lst);
        
        // Build the subderivatives
        while (it->hasNext()) {
            Val u = it->next();
            Val dy = deriveConstVal(id, y, u, 0);

            if (!dy) {
                res->rem_ref();
                delete it;
                return NULL;
            }

            lst->add(lst->size(), dy);
        }
        delete it;

        // This is to ensure an initial condition for data structures:
        // to ensure that an identity is met.
        if (c == 1) resolveIdentity(res);

        return res;
    } else if (typeid(*x) == typeid(DictVal)) {
        auto vit = ((DictVal*) x)->getVals()->iterator();

        auto vals = new Trie<Val>;
        
        Val res = new DictVal(vals);

        while (vit->hasNext()) {
            auto k = vit->next();

            auto V = deriveConstVal(id, y, ((DictVal*) x)->getVals()->get(k), 0);
            if (!V) {
                delete vit;
                delete res;
                return NULL;
            }

            vals->add(k, V);
        }
        
        // GC
        delete vit;
        
        // This is to ensure an initial condition for data structures:
        // to ensure that an identity is met.
        if (c == 1) resolveIdentity(res);

        return res;

    } else if (typeid(*x) == typeid(TupleVal)) {
        Val L = deriveConstVal(id, y, ((TupleVal*) x)->getLeft(), 0);
        if (!L) return NULL;

        Val R = deriveConstVal(id, y, ((TupleVal*) x)->getRight(), 0);
        if (!R) { L->rem_ref(); return NULL; }

        L->add_ref();
        R->add_ref();

        auto res = new TupleVal(L, R);

        if (c == 1) resolveIdentity(res);

        return res;

    } else if (typeid(*x) == typeid(IntVal) || typeid(*x) == typeid(RealVal))
        return deriveConstVal(id, y, c);
    else
        return NULL;
}

Val AndExp::derivativeOf(string x, Env env, Env denv) {
    throw_calc_err(this);
    return NULL;
}
Val OrExp::derivativeOf(string x, Env env, Env denv) {
    throw_calc_err(this);
    return NULL;
}

Val ApplyExp::derivativeOf(string x, Env env, Env denv) {
    // d/dx f(u1(x), u2(x), ...) = df/du1 * du1/dx + df/du2 * du2/dx + ...

    Val o = op->evaluate(env);
    if (!o) return NULL;
    else if (typeid(*o) != typeid(LambdaVal)) {
        throw_type_err(op, "lambda");
        o->rem_ref();
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
                op->symb_diff(func->getArgs()[i]),
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
    
    Val v = deriv->evaluate(env);

    o->rem_ref();

    delete deriv;

    return v;
}

Val DictExp::derivativeOf(string x, Env env, Env denv) {
    // d/dx [u_0, u_1, ...] = [d/dx u_0, d/dx u_1, ...]
    DictVal *val = new DictVal();
    
    // Add each item
    auto kit = keys->iterator();
    auto vit = vals->iterator();
    for(int i = 0; vit->hasNext(); i++) {
        // Compute the value of each item
        Exp exp = vit->next();
        Val v = exp->derivativeOf(x, env, denv);
        if (!v) {
            delete kit;
            delete vit;

            return NULL;
        } else {
            string k = kit->next();
            val->getVals()->add(k, v);
        }
    }

    delete kit;
    delete vit;

    return val;
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

    Val c = exp->evaluate(env);

    delete exp;
    return c;
}

Val ExponentExp::derivativeOf(string x, Env env, Env denv) { 

    Val l = left->evaluate(env);
    if (!l) return NULL;

    Val r = right->evaluate(env);
    if (!r) {
        l->rem_ref();
        return NULL;
    }

    Val dl = left->derivativeOf(x, env, denv);
    if (!dl) {
        l->rem_ref();
        r->rem_ref();
        return NULL;
    }

    Val dr = right->derivativeOf(x, env, denv);
    if (!dr) {
        l->rem_ref();
        r->rem_ref();
        dl->rem_ref();
        return NULL;
    }

    if (val_is_number(r)) {

        // Compute B = gf' + f log f g'
        SumExp sum(NULL, NULL);
        MultExp mult(NULL, NULL);
        
        // g f'
        Val gf = mult.op(r, dl);
        if (!gf) {
            l->rem_ref();
            r->rem_ref();
            dl->rem_ref();
            dr->rem_ref();
            return NULL;
        }
        
        // log f
        Val logf = log(l);
        if (!logf) {
            gf->rem_ref();
            l->rem_ref();
            r->rem_ref();
            dl->rem_ref();
            dr->rem_ref();
            return NULL;
        }
        
        // f log f
        Val flf = mult.op(l, logf);
        logf->rem_ref();
        
        // f log(f) g'
        Val flfg = mult.op(flf, dr);
        flf->rem_ref();

        Val B = sum.op(gf, flfg);
        flfg->rem_ref();
        gf->rem_ref();
        if (!B) {
            l->rem_ref();
            r->rem_ref();
            dl->rem_ref();
            dr->rem_ref();
            return NULL;
        }

        auto g = (val_is_integer(r) ? ((IntVal*) r)->get() : ((RealVal*) r)->get()) - 1;
        RealVal R(g);

        auto A = pow(l, &R);
        l->rem_ref();
        r->rem_ref();
        dl->rem_ref();
        dr->rem_ref();

        if (!A) {
            B->rem_ref();
            return NULL;
        }
        
        auto y = mult.op(A, B);
        A->rem_ref();
        B->rem_ref();

        return y;
    } else {
        l->rem_ref();
        r->rem_ref();
        dl->rem_ref();
        dr->rem_ref();
        throw_err("calculus", "differentiation is not defined in exponentiation between "
                + left->toString() + " and " + right->toString());

        return NULL;
    }

}

Val ForExp::derivativeOf(string x, Env env, Env denv) {
    // Evaluate the list
    Val listExp = set->evaluate(env);
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
    Val b = cond->evaluate(env);

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

    return new LambdaVal(ids, exp->symb_diff(x), env->clone());
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
        Val v = exps[i]->evaluate(env);
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

Val DictAccessExp::derivativeOf(string x, Env env, Env denv) {
    Val lst = list->derivativeOf(x, env, denv);
    if (!lst) return NULL;
    else if (typeid(*lst) != typeid(DictVal)) {
        throw_type_err(list, "list");
        lst->rem_ref();
        return NULL;
    }

    auto trie = ((DictVal*) lst)->getVals();
    
    Val v;
    if (trie->hasKey(idx)) {
        v = trie->get(idx);
        v->add_ref();
    } else
        v = NULL;

    lst->rem_ref();

    return v;
}

Val ListAccessExp::derivativeOf(string x, Env env, Env denv) {
    Val lst = list->derivativeOf(x, env, denv);
    if (!lst) return NULL;
    else if (typeid(*lst) != typeid(ListVal)) {
        throw_type_err(list, "list");
        lst->rem_ref();
        return NULL;
    }

    Val index = idx->evaluate(env);
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
    
    Val v = exp->evaluate(env);

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
    // f(L, c) = g(g(...g(c,L[0]),L[1],...),L[N-1])
    // g'(a,b) = a' * g_a(a,b) + b' * g_b(a,b)
    // f' = g'(g(...(c, L[0]), L[1]), ...), L[N-1])
    //      = g'(g(...(c, L[0]),...), L[N-2]) * g_a(...(c,L[0])...,L[N-1])
    //      + L'[N-1] * g_b(...(c,L[0])...,L[N-1])
    Val lst = list->evaluate(env);
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
    Val f = func->evaluate(env);
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
    
    Val c = base->evaluate(env);
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
            new MultExp(reexpress(fa), reexpress(dc)),
            new MultExp(reexpress(fb), reexpress(v))
        );

        //std::cout << "must compute " << *cell << "\n";

        v = cell->evaluate(env);

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
    Val f = func->evaluate(env);
    if (!f)
        return NULL;
    
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
    
    // Then, we differentiate it.
    string var = fn->getArgs()[0];
    fn->rem_ref();
    f = func->derivativeOf(var, env, denv);
    if (!f) {
        return NULL;
    } else fn = (LambdaVal*) f;
    
    Val vs = list->evaluate(env);
    if (!vs) {
        fn->rem_ref();
        return NULL;
    }

    Val dvs = list->derivativeOf(x, env, denv);
    if (!dvs) {
        // The list could not be differentiated
        fn->rem_ref();
        vs->rem_ref();
        return NULL;
    }

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

                elem = cell->evaluate(env);
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

            v = cell->evaluate(env);
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
    Val l = left->evaluate(env);
    if (!l) { dl->rem_ref(); dr->rem_ref(); return NULL; }
    Val r = right->evaluate(env);
    if (!r) { dl->rem_ref(); dr->rem_ref(); l->rem_ref(); return NULL; }
    
    throw_debug("calculus", "l = " + l->toString());
    throw_debug("calculus", "r = " + r->toString());
    throw_debug("calculus", "d/d" + x + " l = " + dl->toString());
    throw_debug("calculus", "d/d" + x + " r = " + dr->toString());
    
    // Utility object
    SumExp sum(NULL, NULL);
    MultExp mult(NULL, NULL);

    Val a = mult.op(l, dr);
    l->rem_ref();
    dr->rem_ref();

    if (!a) {
        r->rem_ref();
        dl->rem_ref();
        return NULL;
    }

    throw_debug("calculus", "l r' = " + a->toString());

    Val b = mult.op(r, dl);/*(dl, r)*/;
    r->rem_ref();
    dl->rem_ref();

    if (!b) {
        a->rem_ref();
        return NULL;
    }


    throw_debug("calculus", "r l' = " + b->toString());
    
    Val c = sum.op(a, b);

    a->rem_ref();
    b->rem_ref();
    
    if (c) throw_debug("calculus", "l r' + r l' = " + c->toString());
    else throw_debug("calc\x1b[37m_\x1b[31merror", "l r' + r l' is non-computable");

    return c;
}

Val StdMathExp::derivativeOf(string x, Env env, Env denv) {
    Val v = e->evaluate(env);
    if (!v) return NULL;

    bool isnum = val_is_number(v);

    Val dv = e->derivativeOf(x, env, denv);
    if (!dv) return NULL;
    
    MultExp mult(NULL, NULL);
    DivExp div(NULL, NULL);
    Val y;

    RealVal half(0.5);
    IntVal two(2);

    switch (fn) {
        case SIN:
            if (isnum) {
                auto z = typeid(*v) == typeid(IntVal)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                auto dz = typeid(*dv) == typeid(IntVal)
                    ? ((IntVal*) dv)->get()
                    : ((RealVal*) dv)->get();
                return new RealVal(dz*cos(z));
            } else {
                throw_err("type", "sin is undefined for inputs outside of R");
                return NULL;
            }
        case COS:
            if (isnum) {
                auto z = typeid(*v) == typeid(IntVal)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                auto dz = typeid(*dv) == typeid(IntVal)
                    ? ((IntVal*) dv)->get()
                    : ((RealVal*) dv)->get();
                return new RealVal(-dz*sin(z));
            } else {
                throw_err("type", "cos is undefined for inputs outside of R");
                return NULL;
            }
        case LOG:
            y = div.op(dv, v);
            v->rem_ref();
            dv->rem_ref();
            return y;
        case SQRT:
            y = pow(v, &half);
            v->rem_ref();
            if (!y) { dv->rem_ref(); return NULL; }

            v = mult.op(&two, y);
            y->rem_ref();

            y = div.op(dv, v);
            v->rem_ref();

            return y;
        case EXP:
            y - mult.op(v, dv);
            v->rem_ref();
            dv->rem_ref();
            return y;
    }

    throw_err("lomda", "the given math function is undefined");
    return NULL;

}

// d/dx A+B = dA/dx + dB/dx
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

// d/dx (L, R) = (d/dx L, d/dx R)
Val TupleExp::derivativeOf(string x, Env env, Env denv) {
    auto L = left->derivativeOf(x, env, denv);
    if (!L) return NULL;

    auto R = right->derivativeOf(x, env, denv);
    if (!R) { L->rem_ref(); return NULL; }

    return new TupleVal(L, R);
}

// d/dx left of T = left of dT/dx
Val TupleAccessExp::derivativeOf(string x, Env env, Env denv) {
    Val dv = exp->derivativeOf(x, env, denv);

    if (!dv) return NULL;
    else if (typeid(*dv) != typeid(TupleVal)) {
        throw_type_err(exp, "tuple");
        dv->rem_ref();
        return NULL;
    }
    
    TupleVal *tup = (TupleVal*) dv;

    Val Y = idx ? tup->getRight() : tup->getLeft();
    
    // Memory mgt
    Y->add_ref();
    tup->rem_ref();

    return Y;

}

Val VarExp::derivativeOf(string x, Env env, Env denv) {
    // We have a list of derivatives, from which we can easily perform
    // a lookup.
    Val dv = denv->apply(id);
    if (!dv) {
        throw_err("calculus", "derivative of variable '" + id + "' is not known within this context");
        if (VERBOSITY()) throw_debug("runtime error", "error ocurred w/ scope:\n" + denv->toString());
    } else
        dv->add_ref();

    return dv;
}

Val WhileExp::derivativeOf(string x, Env env, Env denv) {
    bool skip = alwaysEnter;
    Val v = new VoidVal;

    while (true) {
        Val c = cond->evaluate(env);
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
    return deriveConstVal(x, env->apply(x), 0);
}
Val RealExp::derivativeOf(string x, Env env, Env denv) {
    return deriveConstVal(x, env->apply(x), 0);
}


