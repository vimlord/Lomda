#include "expressions/derivative.hpp"
#include "expression.hpp"

Expression* Differentiable::applyDenv(DerivEnv denv, std::string id) {
    Iterator<int, DenvFrame> *it  = denv->iterator();
    
    // Search frame by frame for the item.
    while (it->hasNext()) {
        DenvFrame frame = it->next();
        if (frame.id == id) {
            delete it;
            return frame.val;
        }
    }
    
    // The expression was not found in the environment
    delete it;
    return new IntExp;
}

int Differentiable::setDenv(DerivEnv denv, std::string id, Expression *exp) {
    Iterator<int, DenvFrame> *it  = denv->iterator();
    
    // Search frame by frame for the item.
    while (it->hasNext()) {
        DenvFrame frame = it->next();
        if (frame.id == id) {
            delete it;
            frame.val = exp;
            return 0;
        }
    }
    
    // The expression was not found in the environment
    delete it;
    return 1;
}

// For comparing the denv frames (required to make a derivative environment)
bool operator==(const DenvFrame& lhs, const DenvFrame& rhs) {
    return lhs.id == rhs.id && lhs.val == rhs.val;
}

#include "expression.hpp"

void throw_calc_err(Expression* exp) {
    throw_err("runtime", "expression '" + exp->toString() + "' is non-differentiable\n");
}

Expression* ApplyExp::derivativeOf(DerivEnv denv) {
    if (dynamic_cast<const Differentiable*>(op) != nullptr) {
        //Expression *df = ((Differentiable*) op)->derivativeOf(denv);
        LambdaExp *df = (LambdaExp*) ((Differentiable*) op)->derivativeOf(denv);
        if (!df) return NULL;

        std::string *xs = df->getXs();

        int i;
        for (i = 0; xs[i] != ""; i++)
            if (dynamic_cast<const Differentiable*>(args[i]) == nullptr) {
                delete df;
                throw_calc_err(args[i]);
                return NULL;
            }

        std::string *ids = new std::string[i+1];
        Expression **as = new Expression*[i+1];
        ids[i] = ""; as[i] = NULL;

        while (i--) {
            ids[i] = "d" + xs[i] + "/d" + denv->get(0).id;
            as[i] = ((Differentiable*) args[i])->derivativeOf(denv);

            if (!as[i]) {
                while (!as[++i]) delete as[i];
                delete ids, as;
                return NULL;
            }


        }

        Expression *res = new LetExp(ids, as,
            new ApplyExp(df, args));
        return res;
    } else
        return NULL;
}

Expression* DiffExp::derivativeOf(DerivEnv denv) {
    if (dynamic_cast<const Differentiable*>(left) == nullptr
    &&  dynamic_cast<const Differentiable*>(right) == nullptr)
        return NULL;

    Expression *a = ((Differentiable*) left)->derivativeOf(denv);
    Expression *b = ((Differentiable*) right)->derivativeOf(denv);

    // d/dx a - b = da/dx - db/dx
    if (!a || !b) {
        delete a, b;
        return NULL;
    } else
        // d/dx a - b = da/dx + db/dx
        return new DiffExp(a, b);
}

Expression* ForExp::derivativeOf(DerivEnv denv) {
    if (dynamic_cast<const Differentiable*>(body) == nullptr)
        return NULL;
    
    // Compute the per-iteration derivative
    Expression *deriv = ((Differentiable*) body)->derivativeOf(denv);
    
    // Return the outcome
    if (!deriv) return NULL;
    else return new ForExp(id, set->clone(), deriv);
}

Expression* IfExp::derivativeOf(DerivEnv denv) {
    if (dynamic_cast<const Differentiable*>(tExp) == nullptr) {
        throw_calc_err(tExp);
        return NULL;
    }
    if (dynamic_cast<const Differentiable*>(fExp) == nullptr) {
        throw_calc_err(fExp);
        return NULL;
    }

    Expression *t = ((Differentiable*) tExp)->derivativeOf(denv);
    Expression *f = ((Differentiable*) fExp)->derivativeOf(denv);

    if (!t || !f) { delete t, f; return NULL; }
    else return new IfExp(cond, t, f);
}

Expression* LambdaExp::derivativeOf(DerivEnv denv) {
     
    int argc;
    for (argc = 0; xs[argc] != ""; argc++);

    std::string *in = new std::string[argc+1];
    
    // d/dx f(u, v) = u' f_u(u, v) + v' f_v(u, v)

    // Initial condition: the expression has a derivative of zero
    // This could happen if the lambda takes no arguments.
    Differentiable *sum = new IntExp;
    for (int i = 0; i < argc; i++) {
        in[i] = xs[i];

        Expression **vs = new Expression*[2];
        vs[0] = new VarExp(xs[i]);
        vs[1] = NULL;

        Expression *u = new VarExp("d"+xs[i]+"/d"+denv->get(0).id);//VarExp(xs[i]).derivativeOf(denv);
        if (!u) {
            delete[] in, vs;
            delete sum;
            return NULL;
        }

        if (dynamic_cast<const Differentiable*>(exp) == nullptr) {
            delete[] in;
            delete sum, u;
            throw_calc_err(exp);
            return NULL;
        }
            

        // Now, we need to compute f'. This requires
        // that we start a new layer of differentiation.
        DerivEnv denv2 = new LinkedList<DenvFrame>;
        DenvFrame frame;
        frame.id = xs[i];
        frame.val = new IntExp(1);
        denv2->add(0, frame);
    
        // Compute the derivative
        Expression *f = ((Differentiable*) exp)->derivativeOf(denv2);

        // Collect the result
        delete denv2;

        if (!f) {
            delete[] in;
            delete sum, u, f;
            return NULL;
        }
        
        if (i)
            sum = new SumExp(new MultExp(u, f), sum);
        else {
            // Drop the initial condition (it is inefficent to keep)
            delete sum;
            sum = new MultExp(u, f);
        }
    }

    return new LambdaExp(in, sum);

}

Expression* LetExp::derivativeOf(DerivEnv denv) {
    int argc;
    for (argc = 0; exps[argc]; argc++);

    std::string *xs = new std::string[argc+1];
    Expression **es = new Expression*[argc+1];
    
    for (int i = 0; i < argc; i++) {
        xs[i] = ids[i];
        if (dynamic_cast<const Differentiable*>(exps[i]) != nullptr) {
            es[i] = exps[i]->clone();

            // Extend the derivative environment
            DenvFrame frame;
            frame.id = ids[i];
            frame.val = ((Differentiable*) exps[i])->derivativeOf(denv);
            
            if (!frame.val) {
                while (i--) {
                    denv->remove(0);
                    delete es[i];
                }
                delete xs, es;
                return NULL;
            }

            denv->add(0, frame);
        } else {
            throw_calc_err(exps[i]);
            while (i--) denv->remove(0);
            delete xs, es;
            return NULL;
        }
    }
    xs[argc] = "";
    es[argc] = NULL;

    Expression *res;

    if (dynamic_cast<const Differentiable*>(body) != nullptr)
        res = new LetExp(xs, es, ((Differentiable*) body)->derivativeOf(denv));
    else {
        throw_calc_err(body);
        res = NULL;
    }
    
    // Cleanup
    while (argc--) denv->remove(0);

    return res;
}

Expression* ListExp::derivativeOf(DerivEnv denv) {
    // d/dx [u_0, u_1, ...] = [d/dx u_0, d/dx u_1, ...]
    List<Expression*> *dlist = new LinkedList<Expression*>();

    auto it = list->iterator();

    for (int i = 0; it->hasNext(); i++) {
        Expression *ex = it->next();

        if (dynamic_cast<const Differentiable*>(ex) == nullptr) {
            while (!dlist->isEmpty())
                delete dlist->remove(0);
            delete dlist, it;
            throw_calc_err(ex);
            return NULL;
        }

        ex = ((Differentiable*) ex)->derivativeOf(denv);

        if (!ex) {
            while (!dlist->isEmpty())
                delete dlist->remove(0);
            delete dlist, it;
            return NULL;
        } else
            dlist->add(i, ex);
    }
    delete it;

    return new ListExp(dlist);

}

Expression* ListAccessExp::derivativeOf(DerivEnv denv) {
    if (dynamic_cast<const Differentiable*>(list) == nullptr) {
        throw_calc_err(list);
        return NULL;
    }
    Expression *f = ((Differentiable*) list)->derivativeOf(denv);

    if (!f) return NULL;
    else return new ListAccessExp(f, idx);

}

Expression* MultExp::derivativeOf(DerivEnv denv) {
    Expression *a;
    if (dynamic_cast<const Differentiable*>(left) != nullptr)
        a = ((Differentiable*) left)->derivativeOf(denv);
    else {
        throw_calc_err(left);
        return NULL;
    }

    Expression *b;
    if (dynamic_cast<const Differentiable*>(right) != nullptr)
        b = ((Differentiable*) right)->derivativeOf(denv);
    else {
        throw_calc_err(right);
        delete a;
        return NULL;
    }

    // d/dx u v = u v' + u' v
    return new SumExp(
        new MultExp(a, right),
        new MultExp(b, left)
    );
}

Expression* SetExp::derivativeOf(DerivEnv denv) {
    int argc;
    for (argc = 0; exps[argc]; argc++);

    Expression **es = new Expression*[argc+1];
    Expression **vs = new Expression*[argc+1];

    for (int i = 0; i < argc; i++) {
        es[i] = tgts[i]->clone();
        vs[i] = exps[i]->clone();

        if (typeid(*(es[i])) != typeid(VarExp)
        || dynamic_cast<const Differentiable*>(vs[i]) == nullptr) {
            throw_calc_err(vs[i]);
            while (i >= 0) delete es[i], vs[i];
            delete es, vs;
            return NULL;
        }

        Expression *deriv = ((Differentiable*) vs[i])->derivativeOf(denv);
        if (!deriv) {
            while (i >= 0) delete es[i], vs[i];
            delete es, vs;
            return NULL;
        } else if (setDenv(denv, ((VarExp*) es[i])->toString(), deriv)) {
            // The derivative environment could not be adjusted. This means that
            // no record of the variable exists. So, add it.
            DenvFrame frame;
            frame.id = ((VarExp*) es[i])->toString();
            frame.val = deriv;
            denv->add(0, frame);
        }
    }

    return new SetExp(es, vs);

}

Expression* SumExp::derivativeOf(DerivEnv denv) {
    if (dynamic_cast<const Differentiable*>(left) == nullptr) {
        throw_calc_err(left);
        return NULL;
    }
    if (dynamic_cast<const Differentiable*>(right) == nullptr) {
        throw_calc_err(right);
        return NULL;
    }

    Expression *a = ((Differentiable*) left)->derivativeOf(denv);
    Expression *b = ((Differentiable*) right)->derivativeOf(denv);

    if (!a || !b) {
        delete a, b;
        return NULL;
    } else
        // d/dx a + b = da/dx + db/dx
        return new SumExp(a, b);
}

Expression* VarExp::derivativeOf(DerivEnv denv) {
    // We have a list of derivatives, from which we can easily perform
    // a lookup.
    Expression *dx = Differentiable::applyDenv(denv, id);
    return dx->clone();
}

Expression* WhileExp::derivativeOf(DerivEnv denv) {
    // The body must be differentiable
    if (dynamic_cast<const Differentiable*>(body) == nullptr) {
        throw_calc_err(body);
        return NULL;
    }

    Expression *dbody = ((Differentiable*) body)->derivativeOf(denv);
    if (!dbody) {
        delete dbody;
        return NULL;
    }
    
    // Regardless of the while type, the basis of the statement
    // will be a while loop
    Expression *wh = new WhileExp(cond->clone(), dbody, true);
    
    // Whether to conditionally branch depends on whether or not the
    // while symbolizes a do-while
    if (alwaysEnter) return wh;
    else return new IfExp(cond->clone(), wh, new IntExp);
}

