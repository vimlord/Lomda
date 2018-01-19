#ifndef _EXPRESSIONS_STDLIB_HPP_
#define _EXPRESSIONS_STDLIB_HPP_

#include "baselang/expression.hpp"

class PrintExp : public Expression {
    private:
        Exp *args;
    public:
        PrintExp(Exp *l) : args(l) {}
        ~PrintExp() { for (int i = 0; args[i]; i++) delete args[i]; delete[] args; }

        Val valueOf(Env);

        Exp clone();
        std::string toString();

        Exp optimize();
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&);
        int opt_var_usage(std::string x);
};

enum StdlibOp {
    SIN, COS, LOG, SQRT
};
class StdlibOpExp : public Expression {
    private:
        StdlibOp op;
        Exp x;
    public:
        StdlibOpExp(StdlibOp so, Exp e) : op(so), x(e) {}
        ~StdlibOpExp() { delete x; }

        Val valueOf(Env);
        Val derivativeOf(std::string, Env, Env);
        
        Exp clone() { return new StdlibOpExp(op, x->clone()); }
        std::string toString();

        Exp optimize();
        Exp opt_const_prop(opt_varexp_map &vs, opt_varexp_map &end) { x = x->opt_const_prop(vs, end); return this; }
        int opt_var_usage(std::string v) { return x->opt_var_usage(v); }
};

#endif
