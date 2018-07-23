#ifndef _EXPRESSIONS_STDLIB_HPP_
#define _EXPRESSIONS_STDLIB_HPP_

#include "baselang/expression.hpp"
#include "baselang/types.hpp"

class PrintExp : public Expression {
    private:
        Exp *args;
    public:
        PrintExp(Exp *l) : args(l) {}
        ~PrintExp() { for (int i = 0; args[i]; i++) delete args[i]; delete[] args; }

        Val evaluate(Env);
        Type* typeOf(Tenv);

        Exp clone();
        std::string toString();

        Exp optimize();
};

/**
 * An expression that wraps a number of math functions
 */
class StdMathExp : public Expression {
    public:
        enum MathFn {
            // Trig
            SIN, COS, TAN,
            ASIN, ACOS, ATAN,
            // Hyperbolic trig
            SINH, COSH, TANH,
            ASINH, ACOSH, ATANH,
            // Exponentiation
            LOG, SQRT, EXP,
            // Search
            MAX, MIN
        };
    private:
        Exp e;
        MathFn fn;
    public:
        
        StdMathExp(MathFn f, Exp x) : e(x), fn(f) {}
        ~StdMathExp() { delete e; }

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        Exp symb_diff(std::string);

        Type* typeOf(Tenv);
        
        Exp clone() { return new StdMathExp(fn, e->clone()); }
        std::string toString();

        Exp getArg() { return e; }
        MathFn getFn() { return fn; }

        Exp optimize();
};

/**
 * An expression that calls a function
 */
class ImplementExp : public Expression {
    private:
        Val (*f)(Env) = NULL; // The function
        Val (*df)(std::string, Env, Env) = NULL; // The derivative of the function
        Type *type;
        std::string name = "";
    public:
        ImplementExp(Val (*fn)(Env), Type *t = NULL) : f(fn), type(t) {}

        Exp clone() {
            return (new ImplementExp(f, type ? type->clone() : NULL))
                    ->setName(name)
                    ->setDerivative(df);
        }

        Val evaluate(Env env) { return f(env); }
        Val derivativeOf(std::string x, Env env, Env denv);

        
        ImplementExp* setEvaluator(std::string n) { name = n; return this; }
        ImplementExp* setDerivative(Val (*f)(std::string, Env, Env)) { df = f; return this; }
        ImplementExp* setName(std::string n) { name = n; return this; }

        std::string toString() { return name.length() == 0 ? "<c-program>" : name; }
};


#endif
