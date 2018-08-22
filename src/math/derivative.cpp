#include "expression.hpp"
#include "baselang/environment.hpp"
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

    if (isVal<ListVal>(val)) {
        if (!idx) idx = new LinkedList<int>;

        ListVal *lst = (ListVal*) val;
        
        for (int i = 0; i < lst->size(); i++) {
            idx->add(0, i);
            resolveIdentity(lst->get(i), idx);
            idx->remove(0);
        }

        if (idx->size() == 0) delete idx;
    } else if (isVal<DictVal>(val)) {
        if (!idx) idx = new LinkedList<int>;
        
        DictVal *dct = (DictVal*) val;

        auto vit = dct->iterator();
        for (int i = 0; vit->hasNext(); i++) {
            string k = vit->next();
            idx->add(0, i);
            resolveIdentity(dct->get(k), idx);
            idx->remove(0);
        }

        if (idx->size() == 0) delete idx;
    } else if (isVal<TupleVal>(val)) {
        if (!idx) idx = new LinkedList<int>;

        idx->add(0, 0);
        resolveIdentity(((TupleVal*) val)->getLeft(), idx);
        idx->remove(0);

        idx->add(0, 1);
        resolveIdentity(((TupleVal*) val)->getRight(), idx);
        idx->remove(0);

        if (idx->size() == 0) delete idx;
    } else if (idx->size() % 2 == 0) {
        
        // Iterate to check that the indices are symmetric
        int i, j;
        for (
            i = 0, j = idx->size()/2;
            i < idx->size()/2 && idx->get(i) == idx->get(j);
            ++i, ++j
        );
        
        // If the indices are symmetric, then apply to the diagonal
        if (j == idx->size()) {
            Val one = isVal<IntVal>(val) ? (Val) new IntVal(1) : (Val) new RealVal(1);
            val->set(one);
            one->rem_ref();
        }

    }
}

Val deriveConstVal(string id, Val v, int c) {

    if (isVal<StringVal>(v) ||
        isVal<BoolVal>(v))
        // Certain types are non-differentiable
        return NULL;
    else if (isVal<ListVal>(v)) {
        auto lst = (ListVal*) v;

        auto vals = new Val[lst->size()];

        for (int i = 0; i < lst->size(); i++) {
            Val x = deriveConstVal(id, lst->get(i), c);

            if (!x) {
                while (i--) vals[i]->rem_ref();
                delete[] vals;
                return NULL;
            }
            
            vals[i] = x;
        }
        return new ListVal(vals, lst->size());

    } else if (isVal<DictVal>(v)) {
        auto vit = ((DictVal*) v)->iterator();

        auto res = new DictVal;

        while (vit->hasNext()) {
            auto k = vit->next();
            
            res->add(k, deriveConstVal(id, ((DictVal*) v)->get(k), c));
        }

        delete vit;

        return res;

    } else if (isVal<TupleVal>(v)) {
        Val L = deriveConstVal(id, ((TupleVal*) v)->getLeft(), c);
        if (!L) return NULL;

        Val R = deriveConstVal(id, ((TupleVal*) v)->getRight(), c);
        if (!R) { L->rem_ref(); return NULL; }

        L->add_ref();
        R->add_ref();

        return new TupleVal(L, R);

    } else if (isVal<LambdaVal>(v)) {
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
        else env = new Environment;

        return new LambdaVal(ids, body, env);
    } else if (isVal<IntVal>(v) || isVal<RealVal>(v))
        return new IntVal(c);
    else
        return NULL;
}

/**
 * Computes dy/dx ~= c
 */
Val deriveConstVal(string id, Val y, Val x, int c) {

    //return deriveConstVal(id, y, c);

    if (isVal<StringVal>(x) ||
        isVal<BoolVal>(x))
        // Certain types are non-differentiable
        return NULL;
    else if (isVal<ListVal>(x)) {
        auto X = (ListVal*) x;
        
        // Resulting derivative
        auto lst = new Val[X->size()];
        
        // Build the subderivatives
        for (int i = 0; i < X->size(); i++) {
            Val u = X->get(i);
            Val dy = deriveConstVal(id, y, u, 0);

            if (!dy) {
                while (i--) lst[i]->rem_ref();
                delete[] lst;
                return NULL;
            }
            
            lst[i] = dy;
        }

        Val res = new ListVal(lst, X->size());

        // This is to ensure an initial condition for data structures:
        // to ensure that an identity is met.
        if (c == 1) resolveIdentity(res);

        return res;
    } else if (isVal<DictVal>(x)) {
        auto vit = ((DictVal*) x)->iterator();

        auto res = new DictVal;

        while (vit->hasNext()) {
            auto k = vit->next();

            auto V = deriveConstVal(id, y, ((DictVal*) x)->get(k), 0);
            if (!V) {
                delete vit;
                delete res;
                return NULL;
            }

            res->add(k, V);
        }
        
        // GC
        delete vit;
        
        // This is to ensure an initial condition for data structures:
        // to ensure that an identity is met.
        if (c == 1) resolveIdentity(res);

        return res;

    } else if (isVal<TupleVal>(x)) {
        Val L = deriveConstVal(id, y, ((TupleVal*) x)->getLeft(), 0);
        if (!L) return NULL;

        Val R = deriveConstVal(id, y, ((TupleVal*) x)->getRight(), 0);
        if (!R) { L->rem_ref(); return NULL; }

        L->add_ref();
        R->add_ref();

        auto res = new TupleVal(L, R);

        if (c == 1) resolveIdentity(res);

        return res;

    } else if (isVal<IntVal>(x) || isVal<RealVal>(x))
        return deriveConstVal(id, y, c);
    else
        return NULL;
}

/**
 * Computes chain rule; f'(g(x))g'(x)
 * @param dzdy f'(g(x))
 * @param dydx g'(x)
 * @param z
 * @return (f(g(x)))' = f'(g(x))g'(x)
 */
Val chain_product(Val dzdy, Val dydx, Val z) {
    if (val_is_number(z)) {
        // dz/dy is the same dimensions as y
        Val dzdx = dot(dydx, dzdy);
        return dzdx;
    } else if (isVal<ListVal>(z)) {
        auto lst = (ListVal*) dzdy;
        auto Z = (ListVal*) z;
        auto dzdx = new ListVal;

        for (int i = 0; i < Z->size(); i++) {
            Val v = chain_product(lst->get(i), dydx, Z->get(i));

            // Error check
            if (!v) {
                dzdx->rem_ref();
                return NULL;
            }

            dzdx->add(i, v);
        }

        return dzdx;
    } else if (isVal<DictVal>(z)) {
        auto dct = (DictVal*) dzdy;
        auto Z = (DictVal*) z;
        auto dzdx = new DictVal;
        
        auto it = Z->iterator();
        while (it->hasNext()) {
            string i = it->next();
            Val v = chain_product(dct->get(i), dydx, Z->get(i));

            if (!v) {
                dzdx->rem_ref();
                return NULL;
            }

            dzdx->add(i, v);
        }

        return dzdx;
    } else if (isVal<AdtVal>(z)) {
        auto adt = (AdtVal*) dzdy;
        auto Z = (AdtVal*) z;

        int argc = 0;
        for (argc = 0; Z->getArgs()[argc]; argc++);

        Val *args = new Val[argc+1];
        for (int i = 0; i < argc; i++) {
            args[i] = chain_product(adt->getArgs()[i], dydx, Z->getArgs()[i]);

            if (!args[i]) {
                while (i--) args[i]->rem_ref();
                delete[] args;
                return NULL;
            }

        }
        args[argc] = NULL;

        return new AdtVal(Z->getType(), Z->getKind(), args);
    
    } else {
        throw_err("runtime", "cannot apply chain rule through " + dydx->toString() + " and " + dzdy->toString());
        return NULL;
    }
}

Val AndExp::derivativeOf(string, Env, Env) {
    throw_calc_err(this);
    return NULL;
}
Val OrExp::derivativeOf(string, Env, Env) {
    throw_calc_err(this);
    return NULL;
}

/**
 * Performs automatic differentiation of an ADT.
 * Requires that the derivative type be equivalent (for now).
 */
Val AdtExp::derivativeOf(string x, Env env, Env denv) {
    // Determine the argument count.
    int argc;
    for (argc = 0; args[argc]; argc++);
    
    // Build an argument list
    Val *xs = new Val[argc+1];
    xs[argc] = NULL;
    for (int i = 0; i < argc; i++) {
        // The item will be the derivative of that item.
        xs[i] = args[i]->derivativeOf(x, env, denv);
        if (!xs[i]) {
            // Evaluation failed, hence we must return.
            while (i--)
                xs[i]->rem_ref();
            return NULL;
        }
    }
    
    return new AdtVal(name, kind, xs);
}
Val SwitchExp::derivativeOf(string x, Env env, Env denv) {
    // Evaluate the ADT
    Val val = adt->evaluate(env);
    if (!val) return NULL;
    else if (!isVal<AdtVal>(val)) {
        throw_type_err(adt, "ADT");
        val->rem_ref();
        return NULL;
    }

    auto A = (AdtVal*) val;
    
    // Differentiate the ADT
    val = adt->derivativeOf(x, env, denv);
    if (!val) {
        A->rem_ref();
        return NULL;
    }

    auto dA = (AdtVal*) val;
    
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
    
    // Extend the environment
    for (i = 0; i < xs; i++) {
        env->set(ids[i], A->getArgs()[i]);
        denv->set(ids[i], dA->getArgs()[i]);
    }

    // Compute the derivative of the body
    Val dy = body->derivativeOf(x, env, denv);
    
    A->rem_ref();
    dA->rem_ref();
    
    return dy;
}

LambdaVal* derive(LambdaVal *func, string x) {
    // Generate the derivative of the body wrt the ith parameter.
    Exp dbody = func->getBody()->symb_diff(x);

    int argc;
    for (argc = 0; func->getArgs()[argc] != ""; argc++);
    
    // Generate the parameter set.
    string *inputs = new string[argc+1];
    for (int j = 0; j < argc; j++)
        inputs[j] = func->getArgs()[j];
    inputs[argc] = "";
    
    // Build a lambda to compute the partial derivative.
    func->getEnv()->add_ref();
    return new LambdaVal(inputs, dbody, func->getEnv());
}

Val ApplyExp::derivativeOf(string x, Env env, Env denv) {
    // d/dx f(u1(x), u2(x), ...) = df/du1 * du1/dx + df/du2 * du2/dx + ...

    Val o = op->evaluate(env);
    
    if (!o) return NULL;
    else if (!isVal<LambdaVal>(o)) {
        throw_type_err(op, "lambda");
        o->rem_ref();
        return NULL;
    }
    
    int argc;
    for (argc = 0; args[argc]; argc++);

    // We will utilize the function to perform the calculations
    LambdaVal *func = (LambdaVal*) o;
    
    // Evaluate the arguments
    Val *vals = new Val[argc+1];
    for (int i = 0; i < argc; i++) {
        vals[i] = args[i]->evaluate(env);
        if (!vals[i]) {
            // Garbage collect
            while (i--) vals[i]->rem_ref();
            delete[] vals;
            func->rem_ref();
            return NULL;
        }
    }
    vals[argc] = NULL;

    Val y = func->apply(vals);
    
    // We aim to figure out if the variable is one of the arguments.
    Val deriv = NULL;
    bool var_caught = false;
    for (int i = 0; i < argc && !var_caught; i++)
        var_caught = func->getArgs()[i] == x;
    
    // If the variable is not outscoped by the function, then it
    // could have an impact on the expression itself.
    if (!var_caught) {
        // If this variable is known in the function's scope, we take the derivative
        if (func->getEnv()->apply(x)) {
            // Compute the initial derivative
            LambdaVal *df = derive(func, x);
            
            // df/dx
            Val dy = df->apply(vals);
            df->rem_ref();

            // dx/dx
            Val dx = denv->apply(x);
            
            deriv = chain_product(dy, dx, y);
            dy->rem_ref();

            if (!deriv) {
                func->rem_ref();
                y->rem_ref();
                return NULL;
            }
        } else {
            // The derivative is definitely "zero". We must compute it.
            Val dy = func->apply(vals);
            
            // Generate zero.
            deriv = deriveConstVal(x, env->apply(x), y, 0);
            
            // GC
            dy->rem_ref();
        }
    }
    
    for (int i = 0; args[i]; i++) {
        // Compute f'(x)
        LambdaVal *df = derive(func, func->getArgs()[i]);

        // Compute f'(g(x))
        Val dy = df->apply(vals);
        df->rem_ref();
        if (!dy) {
            if (deriv) {
                deriv->rem_ref();
                deriv = NULL;
            }
            break;
        }

        // Compute g'(x)
        Val dx = args[i]->derivativeOf(x, env, denv);
        if (!dx) {
            dy->rem_ref();
            if (deriv) {
                deriv->rem_ref();
                deriv = NULL;
            }
            break;
        }
        
        // Compute f'(g(x)) * g'(x)
        Val dydx = chain_product(dy, dx, y);
        dy->rem_ref();
        dx->rem_ref();
        if (!dydx) {
            if (deriv) {
                deriv->rem_ref();
                deriv = NULL;
            }
            break;
        }
        
        // Update the derivative.
        if (deriv) {
            Val v = add(deriv, dydx);
            dydx->rem_ref();
            deriv->rem_ref();

            deriv = v;

            if (!deriv) break;
        } else {
            deriv = dydx;
        }
    }
     
    if (deriv) throw_debug("calculus", "d/d" + x + " " + toString() + " = " + deriv->toString());

    // GC artifacts
    o->rem_ref();
    y->rem_ref();

    // GC the argument list
    for (int i = 0; i < argc; i++)
        vals[i]->rem_ref();
    delete[] vals;

    return deriv;
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
            val->add(k, v);
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

    Exp a = new ValExp(dl);
    Exp b = new ValExp(dr);
    dl->rem_ref();
    dr->rem_ref();

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
        
        
        
        // g f'
        Val gf = mult(r, dl);
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
        Val flf = mult(l, logf);
        logf->rem_ref();
        
        // f log(f) g'
        Val flfg = mult(flf, dr);
        flf->rem_ref();

        Val B = add(gf, flfg);
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
        
        auto y = mult(A, B);
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
    else if (!isVal<ListVal>(listExp)) {
        throw_type_err(set, "list");
        return NULL;
    }
    // And its derivative
    Val dlistExp = set->derivativeOf(x, env, denv);
    if (!dlistExp) return NULL;

    auto list = (ListVal*) listExp;
    auto dlist = (ListVal*) dlistExp;
    
    // Value to be return
    Val v = new VoidVal;
    
    for (int i = 0; i < list->size(); i++) {
        // Get the next item from the list
        Val v = list->get(i);
        Val dv = dlist->get(i);
        
        Val t = env->apply(id);
        Val dt = denv->apply(id);
        if (t) t->add_ref();
        if (dt) dt->add_ref();
        
        // Adjust the environment
        env->set(id, v);
        denv->set(id, dv);
        
        // Evaluate
        v = body->derivativeOf(x, env, denv);

        // Reset
        env->rem(id);
        denv->rem(id);

        if (t) {
            env->set(id, t);
            t->rem_ref();
        } if (dt) {
            denv->set(id, dt);
            dt->rem_ref();
        }

        if (!v) {
            return NULL;
        }

    }

    return v;

}

Val IfExp::derivativeOf(string x, Env env, Env denv) {
    Val b = cond->evaluate(env);

    if (!isVal<BoolVal>(b)) {
        return NULL;
    }

    BoolVal *B = (BoolVal*) b;
    bool bRes = B->get();

    return (bRes ? tExp : fExp)->derivativeOf(x, env, denv);
}

Val LambdaExp::derivativeOf(string x, Env env, Env) {
    int argc;
    for (argc = 0; xs[argc] != ""; argc++);
    
    // Copy the argument names into new memory
    string *ids = new string[argc+1];
    ids[argc] = "";
    while (argc--)
        ids[argc] = xs[argc];
    
    env->add_ref();
    return new LambdaVal(ids, exp->symb_diff(x), env);
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
                env->rem(ids[i]);
                denv->rem(ids[i]);
            }
            return NULL;
        }
        
        // Add it to the environments
        env->set(ids[i], v->clone());
        denv->set(ids[i], dv);

        // We permit all lambdas to have recursive behavior
        if (isVal<LambdaVal>(v)) {
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
    while (argc--) {
        env->rem(ids[argc]);
        denv->rem(ids[argc]);
    }
        
    // Return the result
    return y;


}

Val ListExp::derivativeOf(string x, Env env, Env denv) {
    // d/dx [u_0, u_1, ...] = [d/dx u_0, d/dx u_1, ...]
    ListVal *val = new ListVal;
    
    // Add each item
    for(int i = 0; i < size(); i++) {
        // Compute the value of each item
        Exp exp = get(i);
        Val v = exp->derivativeOf(x, env, denv);
        if (!v) {
            return NULL;
        } else
            val->add(i, v);
    }

    return val;
}

Val DictAccessExp::derivativeOf(string x, Env env, Env denv) {
    Val lst = list->derivativeOf(x, env, denv);
    if (!lst) return NULL;
    else if (!isVal<DictVal>(lst)) {
        throw_type_err(list, "dict");
        lst->rem_ref();
        return NULL;
    }

    auto dict = (DictVal*) lst;
    
    Val v;
    if (dict->hasKey(idx)) {
        v = dict->get(idx);
        v->add_ref();
    } else
        v = NULL;

    lst->rem_ref();

    return v;
}

Val ImplementExp::derivativeOf(std::string x, Env env, Env denv) {
    if (df) {
        return df(x, env, denv);
    } else {
        throw_err("calculus", "expression '" + toString() + "' is non-differentiable");
        return NULL;
    }
}

Val ListAccessExp::derivativeOf(string x, Env env, Env denv) {
    Val lst = list->derivativeOf(x, env, denv);
    if (!lst) return NULL;
    else if (!isVal<ListVal>(lst)) {
        throw_type_err(list, "list");
        lst->rem_ref();
        return NULL;
    }

    Val index = idx->evaluate(env);
    if (!index) return NULL;
    else if (!isVal<IntVal>(index)) {
        throw_type_err(idx, "integer");
        lst->rem_ref();
        index->rem_ref();
        return NULL;
    }

    Val v = ((ListVal*) lst)->get(((IntVal*) index)->get());

    v->add_ref();
    index->rem_ref();
    lst->rem_ref();

    return v;
}

Val MagnitudeExp::derivativeOf(string x, Env env, Env denv) {
    Val res = NULL;
    
    Val v = exp->evaluate(env);

    if (!v) return NULL;
    else if (isVal<IntVal>(v) || isVal<RealVal>(v)) {
        auto val = (isVal<IntVal>(v))
                ? ((IntVal*) v)->get()
                : ((RealVal*) v)->get();

        v->rem_ref();

        Val dv = exp->derivativeOf(x, env, denv);
        if (dv) {
            if (isVal<IntVal>(dv))
                res = new IntVal((val >= 0 ? 1 : -1) * ((IntVal*) dv)->get());
            else if (isVal<RealVal>(dv))
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

/**
 * Note to readers: this was by far the biggest pain in the ass
 * when it came down to writing out the derivative for this.
 * - Vimlord
 */
Val FoldExp::derivativeOf(string x, Env env, Env denv) {
    // f(L, c) = g(g(...g(c,L[0]),L[1],...),L[N-1])
    // g'(a,b) = a' * g_a(a,b) + b' * g_b(a,b)
    // f' = g'(g(...(c, L[0]), L[1]), ...), L[N-1])
    //      = g'(g(...(c, L[0]),...), L[N-2]) * g_a(...(c,L[0])...,L[N-1])
    //      + L'[N-1] * g_b(...(c,L[0])...,L[N-1])
    Val lst = list->evaluate(env);
    if (!lst) return NULL;
    else if (!isVal<ListVal>(lst)) {
        lst->rem_ref();
        throw_type_err(list, "list");
        return NULL;
    } else if (((ListVal*) lst)->isEmpty()) {
        // Base case: empty list
        lst->rem_ref();
        return base->derivativeOf(x, env, denv);
    }
    
    Val dlst = list->derivativeOf(x, env, denv);
    if (!dlst) return NULL;
    
    // Attempt to evaluate the fold function
    Val f = func->evaluate(env);
    if (!f) return NULL;
    else if (!isVal<LambdaVal>(f)) {
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
        return NULL;
    }
    LambdaVal *df_a = (LambdaVal*) v;

    // The derivative wrt the second var
    v = func->derivativeOf(fn->getArgs()[1], env, denv);
    if (!v) {
        fn->rem_ref();
        lst->rem_ref();
        dlst->rem_ref(); 
        df_a->rem_ref();
        return NULL;
    }
    LambdaVal *df_b = (LambdaVal*) v;

    Val xs[3];
    xs[2] = NULL;
    
    Val c = base->evaluate(env);
    Val dc = base->derivativeOf(x, env, denv);
    
    for (int i = 0; i < ((ListVal*) lst)->size() && c && dc; i++) {
        v = ((ListVal*) lst)->get(i);

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
        v = ((ListVal*) dlst)->get(i);
        
        // We will construct an expression to handle the ordeal
        Expression *cell = new SumExp(
            new MultExp(new ValExp(fa), new ValExp(dc)),
            new MultExp(new ValExp(fb), new ValExp(v))
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
    
    // Then, we differentiate it.
    string var = fn->getArgs()[0];

    fn->rem_ref();

    f = func->derivativeOf(var, env, denv);
    if (!f) {
        return NULL;
    }
    
    fn = (LambdaVal*) f;
    
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

    if (isVal<ListVal>(vs)) {
        // Given a list, map each element of the list
        ListVal *vals = (ListVal*) vs;
        ListVal *dvals = (ListVal*) dvs;

        ListVal *res = new ListVal;
        
        for (int i = 0; i < vals->size(); i++) {
            Val v = vals->get(i);
            Val dv = dvals->get(i);

            Val *xs = new Val[2];
            xs[0] = v;
            xs[1] = NULL;
            
            // f'(x)
            Val elem = fn->apply(xs);

            delete[] xs;

            if (!elem) {
                fn->rem_ref();
                vals->rem_ref();
                dvals->rem_ref();
                res->rem_ref();
                return NULL;
            } else {
                // Compute the other part
                
                Val y = mult(elem, dv);

                elem->rem_ref();

                if (y)
                    // Compute the answer
                    res->add(res->size(), y);
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
        
        // Garbage collection
        fn->rem_ref();
        vals->rem_ref();
        dvals->rem_ref();
        
        return res;

    } else if (configuration.werror) {
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
                new ValExp(v),
                new ValExp(dvs)
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

Val ModulusExp::derivativeOf(string x, Env env, Env denv) {
    Val a = left->derivativeOf(x, env, denv);
    if (!a) return NULL;

    Val b = right->derivativeOf(x, env, denv);
    if (!b) {
        a->rem_ref();
        return NULL;
    }
    
    DivExp div(left->clone(), right->clone());
    
    
    Val aOb = div.evaluate(env);
    if (val_is_real(aOb)) {
        Val I = new IntVal((int) ((RealVal*) aOb)->get());
        aOb->rem_ref();
        aOb = I;
    } else if (!val_is_integer(aOb)) {
        aOb->rem_ref();
        a->rem_ref();
        b->rem_ref();
        throw_err("type", "derivative of modulus is undefined on non-numbers");
        return NULL;
    }

    Val baOb = mult(b, aOb);
    Val c = sub(a, baOb);

    baOb->rem_ref();
    a->rem_ref();
    b->rem_ref();

    return c;
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

    Val a = mult(l, dr);
    l->rem_ref();
    dr->rem_ref();

    if (!a) {
        r->rem_ref();
        dl->rem_ref();
        return NULL;
    }

    Val b = mult(r, dl);/*(dl, r)*/;
    r->rem_ref();
    dl->rem_ref();

    if (!b) {
        a->rem_ref();
        return NULL;
    }

    Val c = add(a, b);

    a->rem_ref();
    b->rem_ref();
    
    return c;
}

Val StdMathExp::derivativeOf(string x, Env env, Env denv) {
    Val v = e->evaluate(env);
    if (!v) return NULL;

    bool isnum = val_is_number(v);
    
    // Is the value a list of numbers
    bool islst = val_is_list(v);
    if (islst) {
        auto lst = (ListVal*) v;

        for (int i = 0; islst && i < lst->size(); i++)
            islst = val_is_number(lst->get(i));
    }

    Val dv = e->derivativeOf(x, env, denv);
    if (!dv) return NULL;
    
    Val y;

    RealVal half(0.5);
    IntVal two(2);
    IntVal one(1);

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
                y = lst->get(0);
                int idx = 0;

                for (int i = 1; i < lst->size(); i++) {
                    Val val = lst->get(i);

                    BoolVal *b = (BoolVal*) gt.op(val, y);
                    if (b->get() == (fn == MAX)) {
                        idx = i;
                        y = val;
                    }
                    b->rem_ref();
                }
                
                y = ((ListVal*) dv)->get(idx);
                y->add_ref();
            } else
                throw_err("type", "max is undefined for inputs outside of [R]");
            break;
        case SIN:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                auto dz = isVal<IntVal>(dv)
                    ? ((IntVal*) dv)->get()
                    : ((RealVal*) dv)->get();
                y = new RealVal(dz*cos(z));
            } else
                throw_err("type", "sin is undefined for inputs outside of R");
            break;
        case COS:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                auto dz = isVal<IntVal>(dv)
                    ? ((IntVal*) dv)->get()
                    : ((RealVal*) dv)->get();
                y = new RealVal(-dz*sin(z));
            } else
                throw_err("type", "cos is undefined for inputs outside of R");
            break;
        case TAN:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                auto dz = isVal<IntVal>(dv)
                    ? ((IntVal*) dv)->get()
                    : ((RealVal*) dv)->get();

                // d/dx tan(x) = sec^2(x)
                y = new RealVal(dz / (cos(z) * cos(z)));
            } else
                throw_err("type", "tan is undefined for inputs outside of R");
            break;
        case ASIN:
            if (isnum) {
                // x^2
                y = pow(v, &two);
                if (!y) break;
                
                // 1 - x^2
                auto dz = sub(&one, y);
                y->rem_ref();
                y = dz;
                if (!y) break;
                
                // sqrt(1 - x^2)
                dz = pow(y, &half);
                y->rem_ref();
                y = dz;
                if (!y) break;
                
                // dx / sqrt(1 - x^2)
                dz = div(dv, y);
                y->rem_ref();
                y = dz;
            } else
                throw_err("type", "arcsin is undefined for inputs outside of R");
            break;
        case ACOS:
            if (isnum) {
                // x^2
                y = pow(v, &two);
                if (!y) break;
                
                // 1 - x^2
                auto dz = sub(&one, y);
                y->rem_ref();
                y = dz;
                if (!y) break;
                
                // sqrt(1 - x^2)
                dz = pow(y, &half);
                y->rem_ref();
                y = dz;
                if (!y) break;
                
                // dx / sqrt(1 - x^2)
                dz = div(dv, y);
                y->rem_ref();
                y = dz;
                if (!y) break;
                
                // -dx / sqrt(1 - x^2)
                IntVal neg(-1);
                dz = mult(&neg, y);
                y->rem_ref();
                y = dz;

            } else
                throw_err("type", "arccos is undefined for inputs outside of R");
            break;
        case ATAN:
            if (isnum) {
                // x^2
                y = pow(v, &two);
                if (!y) break;
                
                // 1 + x^2
                auto dz = add(&one, y);
                y->rem_ref();
                y = dz;
                if (!y) break;
                
                // dx / (1 + x^2)
                dz = div(dv, y);
                y->rem_ref();
                y = dz;
            } else
                throw_err("type", "arctan is undefined for inputs outside of R");
            break;
        case SINH:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                auto dz = isVal<IntVal>(dv)
                    ? ((IntVal*) dv)->get()
                    : ((RealVal*) dv)->get();
                y = new RealVal(dz*cos(z));
            } else
                throw_err("type", "sinh is undefined for inputs outside of R");
            break;
        case COSH:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                auto dz = isVal<IntVal>(dv)
                    ? ((IntVal*) dv)->get()
                    : ((RealVal*) dv)->get();
                y = new RealVal(dz*sin(z));
            } else
                throw_err("type", "cosh is undefined for inputs outside of R");
            break;
        case TANH:
            if (isnum) {
                auto z = isVal<IntVal>(v)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
                auto dz = isVal<IntVal>(dv)
                    ? ((IntVal*) dv)->get()
                    : ((RealVal*) dv)->get();

                // d/dx tanh(x) = sech^2(x)
                y = new RealVal(dz / (cosh(z) * cosh(z)));
            } else
                throw_err("type", "tanh is undefined for inputs outside of R");
            break;
        case ASINH:
            if (isnum) {
                // x^2
                y = pow(v, &two);
                if (!y) break;
                
                // 1 + x^2
                auto dz = add(&one, y);
                y->rem_ref();
                y = dz;
                if (!y) break;
                
                // sqrt(1 + x^2)
                dz = pow(y, &half);
                y->rem_ref();
                y = dz;
                if (!y) break;
                
                // dx / sqrt(1 + x^2)
                dz = div(dv, y);
                y->rem_ref();
                y = dz;

            } else
                throw_err("type", "arcsinh is undefined for inputs outside of R");
            break;
        case ACOSH:
            if (isnum) {
                // x^2
                y = pow(v, &two);
                if (!y) break;
                
                // 1 - x^2
                auto dz = sub(&one, y);
                y->rem_ref();
                y = dz;
                if (!y) break;
                
                // sqrt(1 - x^2)
                dz = pow(y, &half);
                y->rem_ref();
                y = dz;
                if (!y) break;
                
                // dx / sqrt(1 - x^2)
                dz = div(dv, y);
                y->rem_ref();
                y = dz;

            } else
                throw_err("type", "arccosh is undefined for inputs outside of R");
            break;
        case ATANH:
            if (isnum) {
                // x^2
                y = pow(v, &two);
                if (!y) break;
                
                // 1 - x^2
                auto dz = sub(&one, y);
                y->rem_ref();
                y = dz;
                if (!y) break;
                
                // dx / (1 - x^2)
                dz = div(dv, y);
                y->rem_ref();
                y = dz;
            } else
                throw_err("type", "arctanh is undefined for inputs outside of R");
            break;
        case LOG:
            y = div(dv, v);
            break;
        case SQRT:
            y = pow(v, &half);
            if (!y) break;

            v = mult(&two, y);
            y->rem_ref();

            y = div(dv, v);
            break;
        case EXP:
            y = mult(v, dv);
            break;
    }

    v->rem_ref();
    dv->rem_ref();

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
    else if (!isVal<TupleVal>(dv)) {
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

Val VarExp::derivativeOf(string, Env, Env denv) {
    // We have a list of derivatives, from which we can easily perform
    // a lookup.
    Val dv = denv->apply(id);
    if (!dv) {
        throw_err("calculus", "derivative of variable '" + id + "' is not known within this context");
    } else {
        dv->add_ref();
    }

    return dv;
}

Val WhileExp::derivativeOf(string x, Env env, Env denv) {
    bool skip = alwaysEnter;
    Val v = new VoidVal;

    while (true) {
        Val c = cond->evaluate(env);
        if (!c) return NULL;
        else if (!isVal<BoolVal>(c)) {
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
Val IntExp::derivativeOf(string x, Env env, Env) {
    return deriveConstVal(x, env->apply(x), 0);
}
Val RealExp::derivativeOf(string x, Env env, Env) {
    return deriveConstVal(x, env->apply(x), 0);
}


