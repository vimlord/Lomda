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

// An expression that calls a function
class ImplementExp : public Expression {
    private:
        Val (*f)(Env); // The function
        Val (*df)(std::string, Env, Env); // The derivative of the function
        Type *type;
    public:
        ImplementExp(Val (*fn)(Env), Type *t, Val (*dfn)(std::string, Env, Env) = NULL) : f(fn), df(dfn), type(t) {}

        Exp clone() { return new ImplementExp(f, type ? type->clone() : NULL, df); }

        Val evaluate(Env env) { return f(env); }
        Val derivativeOf(std::string x, Env env, Env denv) { return df(x, env, denv); }

        std::string toString() { return "<c-program>"; }
};

// Loads a standard library with a given name, if possible
Type* type_stdlib(std::string);
Val load_stdlib(std::string);

#endif
