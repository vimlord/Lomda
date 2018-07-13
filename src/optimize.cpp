#include "optimize.hpp"
#include "expression.hpp"

using namespace std;

template<typename T>
T* get_from_exp(Exp e) {
    Val v = ((ValExp*) e)->evaluate(NULL);
    
    if (!isVal<T>(v)) {
        v->rem_ref();
        throw "type mismatch";
    } else {
        return (T*) v;
    }
}

Exp optimize_with_catch(Exp orig, Exp mod) {
    try {
        mod = mod->optimize();
        delete orig;
        return mod;
    } catch (std::string x) {
        delete mod;
        throw x;
    }
}

Exp DivExp::optimize() {
    // First, we'll attempt to reduce the load using
    // reduction properties.
    if (isExp<StdMathExp>(left) && isExp<StdMathExp>(right)) {
        // Try reducing mathematical expressions.
        auto A = (StdMathExp*) left;
        auto B = (StdMathExp*) right;
        if (A->getFn() == StdMathExp::MathFn::EXP && B->getFn() == StdMathExp::MathFn::EXP) {
            // e^a * e^b = e^(a+b)
            Exp exp = new StdMathExp(
                    StdMathExp::MathFn::EXP,
                    new DiffExp(A->getArg()->clone(), B->getArg()->clone())
            );
            
            return optimize_with_catch(this, exp);
        }
    }

    left = left->optimize();
    right = right->optimize();

    if (isExp<ValExp>(left) && isExp<ValExp>(right)) {
        // If the left and right hand side are constants, then we
        // can perform constant folding on the operation.
        Val v = evaluate(NULL);
        
        if (!v)
            throw "type mismatch";
        else {
            Exp e = new ValExp(v);
            v->rem_ref();
            return e;
        }
    } else if (isExp<StdMathExp>(left) && isExp<StdMathExp>(right)) {
        // Try reducing mathematical expressions.
        auto A = (StdMathExp*) left;
        auto B = (StdMathExp*) right;
        if (A->getFn() == StdMathExp::MathFn::EXP && B->getFn() == StdMathExp::MathFn::EXP) {
            // e^a * e^b = e^(a+b)
            Exp exp = new StdMathExp(
                    StdMathExp::MathFn::EXP,
                    new DiffExp(A->getArg()->clone(), B->getArg()->clone())
            );

            return optimize_with_catch(this, exp);

        }
    }

    return this;
}

Exp FoldExp::optimize() {
    // Perform base optimization subject to the requirements.
    list = list->optimize();
    func = func->optimize();
    base = base->optimize();

    // In the future, it may be desirable to pre-evaluate the
    // fold here by making use of constants.

    return this;
}
    
Exp ForExp::optimize() {
    set = set->optimize();
    body = body->optimize();
    
    if (isExp<ValExp>(set)) {
        ListVal *L = get_from_exp<ListVal>(set);

        if (L->isEmpty()) {
            // It is empty, therefore it will not iterate
            L->rem_ref();
            return new VoidExp;
        } else {
            L->rem_ref();
            return this;
        }

    } else if (isExp<ListExp>(set) && ((ListExp*) set)->size() == 0) {
        // If the list is always an empty list, then the body will never execute.
        delete this;
        return new VoidExp;
    } else
        return this;

}

Exp IfExp::optimize() {
    cond = cond->optimize();
    
    if (isExp<ValExp>(cond)) {
        BoolVal *B = get_from_exp<BoolVal>(cond);

        B->rem_ref();
        bool b = ((BoolVal*) B)->get();
        
        // The new statement
        Exp y;
        
        // We can simplify
        if (b) {
            throw_debug("preprocessor", "simplified if-then-else conditional to true");
            
            // Reduce to the optimized subbody
            y = tExp->optimize();
            tExp = NULL;
        } else {
            throw_debug("preprocessor", "simplified if-then-else conditional to false");

            // Reduce to the optimized subbody
            y = fExp->optimize();
            fExp = NULL;
        }
        
        // Return
        delete this;
        return y;
    } else {
        
        // If the expression is a not, then flip the bodies.
        if (isExp<NotExp>(cond)) {
            Exp e = tExp;
            tExp = fExp;
            fExp = e;
        }

        tExp = tExp->optimize();
        fExp = fExp->optimize();
        return this;
    }
}
    
Exp LetExp::optimize() {
    // Simplify each of the arguments
    int i;
    for (i = 0; exps[i]; i++) {
        exps[i] = exps[i]->optimize();
    }
    
    // Then, we do plain optimization.
    body = body->optimize();

    if (isExp<ValExp>(body)) {
        // The body simplified to a constant.
        Exp e = body->clone();
        delete this;
        return e;
    } else {
        return this;
    }
}

Exp ListExp::optimize() {
    bool is_const = true;

    // Optimize each element one by one.
    for (int i = 0; i < size(); i++) {
        // Update the index.
        set(i, get(i)->optimize());
        
        // Update our checker for constancy
        is_const &= isExp<ValExp>(get(i));
    }

    if (is_const) {
        // We can evaluate the list and return it because it is constant.
        ValExp *exp = new ValExp(evaluate(NULL));
        delete this;
        return exp;
    } else {
        return this;
    }
}
    
Exp ListAccessExp::optimize() {
    list = list->optimize();
    idx = idx->optimize();

    if (isExp<ListExp>(list) && isExp<IntExp>(idx)) {
        int i = ((IntExp*) idx)->get();
        auto lst = ((ListExp*) list);
        if (i >= 0 && i < lst->size()) {
            Exp e = lst->remove(i);
            delete this;
            return e;
        } else {
            return this;
        }
    }
}

Exp MultExp::optimize() {
    // First, we'll attempt to reduce the load using
    // reduction properties.
    if (isExp<StdMathExp>(left) && isExp<StdMathExp>(right)) {
        // Try reducing mathematical expressions.
        auto A = (StdMathExp*) left;
        auto B = (StdMathExp*) right;
        if (A->getFn() == StdMathExp::MathFn::EXP && B->getFn() == StdMathExp::MathFn::EXP) {
            // e^a * e^b = e^(a+b)
            Exp exp = new StdMathExp(
                    StdMathExp::MathFn::EXP,
                    new SumExp(A->getArg()->clone(), B->getArg()->clone())
            );
            
            return optimize_with_catch(this, exp);
        }
    }

    left = left->optimize();
    right = right->optimize();

    if (isExp<ValExp>(left) && isExp<ValExp>(right)) {
        // If the left and right hand side are constants, then we
        // can perform constant folding on the operation.
        Val v = evaluate(NULL);
        
        if (!v)
            throw "type mismatch";
        else {
            Exp e = new ValExp(v);
            v->rem_ref();
            return e;
        }
    } else if (isExp<StdMathExp>(left) && isExp<StdMathExp>(right)) {
        // Try reducing mathematical expressions.
        auto A = (StdMathExp*) left;
        auto B = (StdMathExp*) right;
        if (A->getFn() == StdMathExp::MathFn::EXP && B->getFn() == StdMathExp::MathFn::EXP) {
            // e^a * e^b = e^(a+b)
            Exp exp = new StdMathExp(
                    StdMathExp::MathFn::EXP,
                    new SumExp(A->getArg()->clone(), B->getArg()->clone())
            );

            return optimize_with_catch(this, exp);

        }
    }

    return this;
}

Exp NotExp::optimize() {
    exp = exp->optimize();

    if (isExp<ValExp>(exp)) {
        // We can directly compute the value
        BoolVal *v = get_from_exp<BoolVal>(exp);

        delete this;
        
        // Compute the negation
        return new ValExp(new BoolVal(!v->get()));
    } else {
        return this;
    }
}

Exp OperatorExp::optimize() {
    left = left->optimize();
    right = right->optimize();

    if (isExp<ValExp>(left) && isExp<ValExp>(right)) {
        // If the left and right hand side are constants, then we
        // can perform constant folding on the operation.
        Val v = evaluate(NULL);
        
        if (!v)
            throw "type mismatch";
        else {
            Exp e = new ValExp(v);
            v->rem_ref();
            return e;
        }
    } else
        return this;
}

Exp PrintExp::optimize() {
    int i;
    for (i = 0; args[i]; i++) {
        args[i] = args[i]->optimize();

        if (isExp<ValExp>(args[i])) {
            // Precompute the string
            string s = args[i]->toString();
            delete args[i];
            args[i] = new ValExp(new StringVal(s));
        }
    }

    return this;
}

Exp PrimitiveExp::optimize() {
    Exp e = new ValExp(evaluate(NULL));
    delete this; 
    return e;
}

Exp DictExp::optimize() {
    auto vs = new LinkedList<Exp>;
    
    while (!vals->isEmpty()) {
        Exp v = vals->remove(0)->optimize();
        
        // Add to the new list
        vs->add(vs->size(), v);
    }
    delete vals;

    vals = vs;

    return this;
}

Exp DictAccessExp::optimize() {
    list = list->optimize();
}

Exp SetExp::optimize() {
    tgt->optimize();
    exp->optimize();
    return this;
}

Exp SequenceExp::optimize() {

    for (int i = 0; i < seq->size(); i++) {
        // Optimize the next element
        Exp exp = seq->get(0)->optimize();

        // We can safely remove it
        seq->remove(0);
        
        if (!isExp<ValExp>(exp) || i == seq->size()-1)
            // If we need the expression, append it to the end.
            seq->add(seq->size(), exp);
        else {
            // Otherwise, we can destroy it.
            delete exp;
            i--;
        }
    }

    if (seq->size() == 1) {
        // If there is only one statement, that is all we need.
        Exp exp = seq->remove(0);
        delete this;
        return exp;
    }

    return this;
}

Exp StdMathExp::optimize() {
    // Optimize the argument
    // If the argument is known, we can compute the result now.
    if (isExp<ValExp>(e)) {
        Val v = evaluate(NULL);
        
        if (v) {
            delete this;
            return new ValExp(v);
        } else {
            throw "execution failure";
        }
    } else if (isExp<StdMathExp>(e)) {
        StdMathExp *g = (StdMathExp*) e;
        
        // Composition with inverse can be reduced out
        if ((fn == LOG && g->fn == EXP)
        ||  (fn == EXP && g->fn == LOG)
        ||  (fn == SIN && g->fn == ASIN)
        ||  (fn == ASIN && g->fn == SIN)
        ||  (fn == COS && g->fn == ACOS)
        ||  (fn == ACOS && g->fn == COS)
        ||  (fn == TAN && g->fn == ATAN)
        ||  (fn == ATAN && g->fn == TAN)
        ||  (fn == SINH && g->fn == ASINH)
        ||  (fn == ASINH && g->fn == SINH)
        ||  (fn == COSH && g->fn == ACOSH)
        ||  (fn == ACOSH && g->fn == COSH)
        ||  (fn == TANH && g->fn == ATANH)
        ||  (fn == ATANH && g->fn == TANH)) {
            Exp res = g->e->clone();
            delete this;
            return res;
        }

        switch (fn) {
            case SQRT:
                if (g->fn == EXP) {
                    // Extract the argument from below. We will use it.
                    Exp x = g->e->clone();
                    delete e;

                    // sqrt(exp(x)) = exp(x/2)
                    fn = EXP;
                    e = new MultExp(new ValExp(new RealVal(0.5)), x);
                    return optimize();
                }
                break;
            case LOG:
                if (g->fn == SQRT) {
                    // Get the argument
                    Exp x = g->e->clone();
                    
                    // Generate what should become the result.
                    Exp exp = new MultExp(new ValExp(new RealVal(0.5)), new StdMathExp(LOG, x));
                    
                    return optimize_with_catch(this, exp);
                }
                break;
        }
    } else if (fn == LOG && isExp<ExponentExp>(e)) {
        // ln(a^b) = b ln a
        ExponentExp *pow = (ExponentExp*) e;
        Exp exp = new MultExp(pow->getRight()->clone(), new StdMathExp(LOG, pow->getLeft()->clone()));
        
        return optimize_with_catch(this, exp);
    } else {
        // Because there were no immediate reductions, we will reduce the argument.
        Exp x = e->optimize();

        if (x != e) {
            // It was reduced, therefore we should search for better optima.
            return optimize();
        } else {
            // We are done>
            return this;
        }
    }
}

Exp SumExp::optimize() {
    // First, we'll attempt to reduce the load using
    // reduction properties.
    if (isExp<StdMathExp>(left) && isExp<StdMathExp>(right)) {
        // Try reducing mathematical expressions.
        auto A = (StdMathExp*) left;
        auto B = (StdMathExp*) right;
        if (A->getFn() == StdMathExp::MathFn::LOG && B->getFn() == StdMathExp::MathFn::LOG) {
            // log a + log b = log ab
            Exp exp = new StdMathExp(
                    StdMathExp::MathFn::LOG,
                    new MultExp(A->getArg()->clone(), B->getArg()->clone())
            );
            
            return optimize_with_catch(this, exp);
        }
    }

    left = left->optimize();
    right = right->optimize();

    if (isExp<ValExp>(left) && isExp<ValExp>(right)) {
        // If the left and right hand side are constants, then we
        // can perform constant folding on the operation.
        Val v = evaluate(NULL);
        
        if (!v)
            throw "type mismatch";
        else {
            Exp e = new ValExp(v);
            v->rem_ref();
            return e;
        }
    } else if (isExp<StdMathExp>(left) && isExp<StdMathExp>(right)) {
        // Try reducing mathematical expressions.
        auto A = (StdMathExp*) left;
        auto B = (StdMathExp*) right;
        if (A->getFn() == StdMathExp::MathFn::LOG && B->getFn() == StdMathExp::MathFn::LOG) {
            // log a + log b = log ab
            Exp exp = new StdMathExp(
                    StdMathExp::MathFn::LOG,
                    new MultExp(A->getArg()->clone(), B->getArg()->clone())
            );
            
            return optimize_with_catch(this, exp);
        }
    }

    return this;
}

Exp WhileExp::optimize() {
    cond = cond->optimize();

    if (isExp<FalseExp>(cond)) {
        // The condition is always false, hence it will never execute.
        delete this;
        return new VoidExp;
    } else {
        body = body->optimize();
        return this;
    }
}

