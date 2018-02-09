#ifndef _EXPRESSIONS_STDLIB_HPP_
#define _EXPRESSIONS_STDLIB_HPP_

#include "baselang/expression.hpp"

class PrintExp : public Expression {
    private:
        Exp *args;
    public:
        PrintExp(Exp *l) : args(l) {}
        ~PrintExp() { for (int i = 0; args[i]; i++) delete args[i]; delete[] args; }

        Val evaluate(Env);

        Exp clone();
        std::string toString();

        Exp optimize();
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&);
        int opt_var_usage(std::string x);
};

// An expression that calls a function
class ImplementExp : public Expression {
    private:
        Val (*f)(Env); // The function
        Val (*df)(std::string, Env, Env); // The derivative of the function
    public:
        ImplementExp(Val (*fn)(Env), Val (*dfn)(std::string, Env, Env) = NULL) : f(fn), df(dfn) {}

        Exp clone() { return new ImplementExp(f, df); }

        Val evaluate(Env env) { return f(env); }
        Val derivativeOf(std::string x, Env env, Env denv) { return df(x, env, denv); }

        std::string toString() { return "<c-program>"; }
};

// Loads a standard library with a given name, if possible
Val load_stdlib(std::string);

#endif
