#ifndef _EXPRESSIONS_PRIMITIVE_HPP_
#define _EXPRESSIONS_PRIMITIVE_HPP_

#include "baselang/expression.hpp"
#include "value.hpp"

// Dictionary generator
class DictExp : public Expression {
    private:
        LinkedList<std::string> *keys;
        LinkedList<Exp> *vals;
    public:
        DictExp() : keys(new LinkedList<std::string>), vals(new LinkedList<Exp>) {}
        DictExp(LinkedList<std::string> *ks, LinkedList<Exp> *vs) : keys(ks), vals(vs) {}
        
        ~DictExp() {
            delete keys;
            while (!vals->isEmpty()) delete vals->remove(0);
            delete vals;
        }

        Val valueOf(Env);
        
        Exp clone();
        std::string toString();
};

// Generates false
class FalseExp : public Expression {
    public:
        FalseExp() {}
        Val valueOf(Env);
        
        Exp clone() { return new FalseExp(); }
        std::string toString() { return "false"; }

        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&) { return this; }
        int opt_var_usage(std::string) { return 0; }
};

// Expression for an integer.
class IntExp : public Expression {
    private:
        int val;
    public:
        IntExp(int = 0);
        Val valueOf(Env);
        Val derivativeOf(std::string, Env, Env);

        int get() { return val; }
        
        Exp clone() { return new IntExp(val); }
        std::string toString();

        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&) { return this; }
        int opt_var_usage(std::string) { return 0; }
};

// Expression that yields closures (lambdas)
class LambdaExp : public Expression {
    private:
        std::string *xs;
        Exp exp;
    public:
        LambdaExp(std::string*, Exp);
        ~LambdaExp() { delete[] xs; delete exp; }
        Val valueOf(Env);
        Val derivativeOf(std::string, Env, Env);

        std::string *getXs() { return xs; }
        
        Exp clone();
        std::string toString();

        Exp optimize() { exp = exp->optimize(); return this; }
        int opt_var_usage(std::string);
};

class ListExp : public Expression {
    private:
        List<Exp> *list;
    public:
        ListExp() : list(new LinkedList<Exp>) {}
        ~ListExp() {
            while (!list->isEmpty()) delete list->remove(0);
            delete list;
        }

        ListExp(Exp*);
        ListExp(List<Exp>* l) : list(l) {}

        Val valueOf(Env);
        Val derivativeOf(std::string, Env, Env);
        
        Exp clone();
        std::string toString();

        List<Exp>* getList() { return list; }
        
        Exp optimize();
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&);
        int opt_var_usage(std::string);
};

// Expression for a real number.
class RealExp : public Expression {
    private:
        float val;
    public:
        RealExp(float = 0);
        Val valueOf(Env);
        Val derivativeOf(std::string, Env, Env);
        
        Exp clone() { return new RealExp(val); }
        std::string toString();

        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&) { return this; }
        int opt_var_usage(std::string) { return 0; }
};

class StringExp : public Expression {
    private:
        std::string val;
    public:
        StringExp(std::string s) : val(s) {}

        Val valueOf(Env) { return new StringVal(val); }

        Exp clone() { return new StringExp(val); }
        std::string toString();

        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&) { return this; }
        int opt_var_usage(std::string) { return 0; }
};

// Generates true
class TrueExp : public Expression {
    public:
        TrueExp() {}
        Val valueOf(Env);
        
        Exp clone() { return new TrueExp(); }
        std::string toString() { return "true"; }

        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&) { return this; }
        int opt_var_usage(std::string) { return 0; }
};

// Get the value of a variable
class VarExp : public Expression {
    private:
        std::string id;
    public:
        VarExp(std::string s) : id(s) {}

        Val valueOf(Env env);
        
        Exp clone() { return new VarExp(id); }
        std::string toString();

        Val derivativeOf(std::string, Env, Env);

        /**
         * Constant propagation will trivially replace the variable if
         * the variable name matches.
         */
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&);
        int opt_var_usage(std::string x) { return x == id ? 2 : 0; }
};

class VoidExp : public Expression {
    public:
        VoidExp() {}

        Val valueOf(Env) { return new VoidVal; }
        std::string toString() { return "void"; }

        Exp clone() { return new VoidExp; }
        
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&) { return this; }
        int opt_var_usage(std::string) { return 0; }
};

#endif
