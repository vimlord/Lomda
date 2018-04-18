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
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&);
        int opt_var_usage(std::string x);
};

// An expression that wraps a number of math functions
class StdMathExp : public Expression {
    public:
        enum MathFn {
            SIN, COS, LOG, SQRT
        };
    private:
        Exp exp;
        MathFn fn;
    public:
        
        StdMathExp(MathFn f, Exp x) : exp(x), fn(f) {}
        ~StdMathExp() { delete exp; }

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        Exp symb_diff(std::string);

        Type* typeOf(Tenv);
        
        Exp clone() { return new StdMathExp(fn, exp->clone()); }
        std::string toString();

        Exp optimize() { exp = exp->optimize(); return this; }
        int opt_var_usage(std::string x) { return exp->opt_var_usage(x); }
};

// An expression that calls a function
class ImplementExp : public Expression {
    private:
        Val (*f)(Env); // The function
        Val (*df)(std::string, Env, Env); // The derivative of the function
        Type *type;
        std::string name = "";
    public:
        ImplementExp(Val (*fn)(Env), Type *t, Val (*dfn)(std::string, Env, Env) = NULL) : f(fn), df(dfn), type(t) {}

        Exp clone() { return new ImplementExp(f, type ? type->clone() : NULL, df); }

        Val evaluate(Env env) { return f(env); }
        Val derivativeOf(std::string x, Env env, Env denv) { return df(x, env, denv); }

        void setName(std::string n) { name = n; }
        std::string toString() { return name.length() == 0 ? "<c-program>" : name; }
};

// Loads a standard library with a given name, if possible
Type* type_stdlib(std::string);
Val load_stdlib(std::string);

#endif
