#ifndef _EXPRESSION_HPP
#define _EXPRESSION_HPP

#include "baselang/expression.hpp"

#include "expressions/operator.hpp"
#include "expressions/primitive.hpp"
#include "expressions/list.hpp"
#include "expressions/stdlib.hpp"

Exp reexpress(Val);

// Calling functions; {a->b, a} -> b
class ApplyExp : public Expression {
    private:
        Exp op;
        Exp *args;
    public:
        ApplyExp(Exp, Exp*);
        ~ApplyExp();

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        
        Exp clone();
        std::string toString();
        int opt_var_usage(std::string);
};

// Casting values; {type t, a} -> t
class CastExp : public Expression {
    private:
        std::string type;
        Exp exp;
    public:
        CastExp(std::string t, Exp e) : type(t), exp(e) {}
        ~CastExp() { delete exp; }

        Val evaluate(Env);

        Exp clone() { return new CastExp(type, exp->clone()); }
        std::string toString();

        Exp optimize() { exp->optimize(); return this; }
        int opt_var_usage(std::string x) { return exp->opt_var_usage(x); }
};

class DerivativeExp : public Expression {
    private:
        Exp func;
        std::string var;
    public:
        DerivativeExp(Exp f, std::string s) : func(f), var(s) {}
        ~DerivativeExp() { delete func; }

        Val evaluate(Env);

        Exp clone() { return new DerivativeExp(func->clone(), var); }
        std::string toString();
};

class FoldExp : public Expression {
    private:
        Exp list;
        Exp func;
        Exp base;
    public:
        FoldExp(Exp l, Exp f, Exp b)
                : list(l), func(f), base(b) {}
        ~FoldExp() { delete list; delete func; delete base; }
        
        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);

        Exp clone() { return new FoldExp(list->clone(), func->clone(), base->clone()); }
        std::string toString();

        Exp optimize();
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&);
        int opt_var_usage(std::string x);
};

class ForExp : public Expression {
    private:
        std::string id;
        Exp set;
        Exp body;
    public:
        ForExp(std::string x, Exp xs, Exp e) : id(x), set(xs), body(e) {}
        ~ForExp() { delete set; delete body; }
        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);

        Exp clone() { return new ForExp(id, set->clone(), body->clone()); }
        std::string toString();

        Exp optimize();
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&);
        int opt_var_usage(std::string x);
};

// Condition expression that chooses paths
class IfExp : public Expression {
    private:
        Exp cond;
        Exp tExp;
        Exp fExp;
    public:
        IfExp(Exp, Exp, Exp);
        ~IfExp() { delete cond; delete tExp; delete fExp; }

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        
        Exp clone() { return new IfExp(cond->clone(), tExp->clone(), fExp->clone()); }
        std::string toString();

        Exp optimize();
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&);
        int opt_var_usage(std::string x);
};

class ImportExp : public Expression {
    private:
        std::string module;
        std::string name;
        Exp exp;
    public:
        // import m
        ImportExp(std::string m, Exp e) : ImportExp(m, m, e) {}
        // import m as n
        ImportExp(std::string m, std::string n, Exp e)
                : module(m), name(n), exp(e) {
                    // Default behavior: return the expression
                    if (!e) exp = new VarExp(m);
        }

        Val evaluate(Env);

        Exp clone() { return new ImportExp(module, name, exp->clone()); }
        std::string toString();
};

class InputExp : public Expression {
    public:
        InputExp() {}
        
        Val evaluate(Env);

        Exp clone() { return new InputExp; }
        std::string toString() { return "input"; }

        int opt_var_usage(std::string x) { return 0; }
};

// Given an expression and a string denoting the type, decide
// whether or not the types match.
class IsaExp : public Expression {
    private:
        Exp exp;
        std::string type;
    public:
        IsaExp(Exp e, std::string t) : exp(e), type(t) {}
        ~IsaExp() { delete exp; }

        Val evaluate(Env);

        Exp clone() { return new IsaExp(exp->clone(), type); }
        std::string toString();

        Exp optimize() { exp = exp->optimize(); return this; }
        Exp opt_const_prop(opt_varexp_map& a, opt_varexp_map& b) { exp = exp->opt_const_prop(a, b); return this; }
        int opt_var_usage(std::string x) { return exp->opt_var_usage(x); }
};

// Expression for defining variables
class LetExp : public Expression {
    private:
        std::string *ids;
        Exp *exps;
        Exp body;
    public:
        LetExp(std::string*, Exp*, Exp);
        ~LetExp() {
            for (int i = 0; exps[i]; i++) delete exps[i];
            delete[] exps;
            delete body;
            delete[] ids;
        }

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        
        Exp clone();
        std::string toString();

        Exp optimize();
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&);
        int opt_var_usage(std::string x);
};

class MagnitudeExp : public Expression {
    private:
        Exp exp;
    public:
        MagnitudeExp(Exp e) : exp(e) {}
        ~MagnitudeExp() { delete exp; }
        Exp clone() { return new MagnitudeExp(exp->clone()); }

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);

        std::string toString();
        
        Exp optimize() { exp = exp->optimize(); return this; }
        int opt_var_usage(std::string x) { return exp->opt_var_usage(x); }
};

class MapExp : public Expression {
    private:
        Exp func;
        Exp list;
    public:
        MapExp(Exp f, Exp l) : func(f), list(l) {}
        ~MapExp() { delete func; delete list; }
        Exp clone() { return new MapExp(list->clone(), func->clone()); }

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        
        std::string toString();

        Exp optimize() { func = func->optimize(); list = list->optimize(); return this; }
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&);
        int opt_var_usage(std::string x) { return func->opt_var_usage(x) | list->opt_var_usage(x); }
};

class NormExp : public Expression {
    private:
        Exp exp;
    public:
        NormExp(Exp e) : exp(e) {}
        ~NormExp() { delete exp; }
        Exp clone() { return new NormExp(exp->clone()); }

        Val evaluate(Env);
        //Val derivativeOf(std::string, Env, Env);

        std::string toString();

        Exp optimize() { exp = exp->optimize(); return this; }
        int opt_var_usage(std::string x) { return exp->opt_var_usage(x); }
};

// Bool -> Bool expression that negates booleans
class NotExp : public Expression {
    private:
        Exp exp;
    public:
        NotExp(Exp e) { exp = e; }
        ~NotExp() { delete exp; }

        Val evaluate(Env);
        
        Exp clone() { return new NotExp(exp->clone()); }
        std::string toString();

        Exp optimize();
        int opt_var_usage(std::string x) { return exp->opt_var_usage(x); }
};

class SequenceExp : public Expression {
    private:
        LinkedList<Exp> *seq;
    public:
        SequenceExp() : seq(new LinkedList<Exp>) {}
        SequenceExp(LinkedList<Exp>* es) : seq(es) {}
        ~SequenceExp() { while (!seq->isEmpty()) delete seq->remove(0); delete seq; }

        LinkedList<Exp>* getSeq() { return seq; }

        Val evaluate(Env);
        
        Exp clone();
        std::string toString();

        Exp optimize();
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&);
        int opt_var_usage(std::string x);
};

// Expression for redefining values in a store
class SetExp : public Expression {
    private:
        Exp tgt;
        Exp exp;
    public:
        SetExp(Exp, Exp);
        ~SetExp() {
            delete tgt;
            delete exp;
        }

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        
        Exp clone();
        std::string toString();

        Exp optimize();
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&);
        int opt_var_usage(std::string x) { return exp->opt_var_usage(x) | (tgt->opt_var_usage(x) >> 1); }
};

class ThunkExp : public Expression {
    private:
        Exp exp;
    public:
        ThunkExp(Exp e) : exp(e) {}
        ~ThunkExp() { delete exp; }

        Val evaluate(Env env) { env->add_ref(); return new Thunk(exp->clone(), env); }
        Val derivativeOf(std::string x, Env e, Env d) { return exp->derivativeOf(x, e, d); }

        Exp optimize() { exp = exp->optimize(); return this; }
        Exp clone() { return new ThunkExp(exp->clone()); }
        std::string toString();
};

/**
 * An anti-thunk. Evaluates to the stored value.
 */
class ValExp : public Expression {
    private:
        Val val;
    public:
        ValExp(Val v) : val(v) {}
        ~ValExp() { val->rem_ref(); }

        Val evaluate(Env) { val->add_ref(); return val; }

        Exp clone() { val->add_ref(); return new ValExp(val); }
        std::string toString();
};

class WhileExp : public Expression {
    private:
        Exp cond;
        Exp body;
        bool alwaysEnter; /* If true, always enter for at least one iteration; do-while */
    public:
        WhileExp(Exp c, Exp b, bool enter = false)
                : cond(c), body(b), alwaysEnter(enter) {}
        ~WhileExp() { delete cond; delete body; }

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);

        Exp clone() { return new WhileExp(cond->clone(), body->clone(), alwaysEnter); }
        std::string toString();

        Exp optimize();
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&);
        int opt_var_usage(std::string x) { return cond->opt_var_usage(x) | body->opt_var_usage(x); }
};

#endif
