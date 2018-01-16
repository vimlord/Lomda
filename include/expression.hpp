#ifndef _EXPRESSION_HPP
#define _EXPRESSION_HPP

#include "baselang/expression.hpp"

#include "expressions/operator.hpp"
#include "expressions/primitive.hpp"
#include "expressions/list.hpp"
#include "expressions/stdlib.hpp"

// Calling functions; {a->b, a} -> b
class ApplyExp : public Expression {
    private:
        Exp op;
        Exp *args;
    public:
        ApplyExp(Exp, Exp*);
        ~ApplyExp();

        Val valueOf(Env);
        Val derivativeOf(std::string, Env, Env);
        
        Exp clone();
        std::string toString();
};

// Casting values; {type t, a} -> t
class CastExp : public Expression {
    private:
        std::string type;
        Exp exp;
    public:
        CastExp(std::string t, Exp e) : type(t), exp(e) {}
        ~CastExp() { delete exp; }

        Val valueOf(Env);

        Exp clone() { return new CastExp(type, exp->clone()); }
        std::string toString();
};

class DerivativeExp : public Expression {
    private:
        Exp func;
        std::string var;
    public:
        DerivativeExp(Exp f, std::string s) : func(f), var(s) {}
        ~DerivativeExp() { delete func; }

        Val valueOf(Env);

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
        
        Val valueOf(Env);
        Val derivativeOf(std::string, Env, Env);

        Exp clone() { return new FoldExp(list->clone(), func->clone(), base->clone()); }
        std::string toString();
};

class ForExp : public Expression {
    private:
        std::string id;
        Exp set;
        Exp body;
    public:
        ForExp(std::string x, Exp xs, Exp e) : id(x), set(xs), body(e) {}
        ~ForExp() { delete set; delete body; }
        Val valueOf(Env);
        Val derivativeOf(std::string, Env, Env);

        Exp clone() { return new ForExp(id, set->clone(), body->clone()); }
        std::string toString();
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

        Val valueOf(Env);
        Val derivativeOf(std::string, Env, Env);
        
        Exp clone() { return new IfExp(cond->clone(), tExp->clone(), fExp->clone()); }
        std::string toString();
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

        Val valueOf(Env);
        Val derivativeOf(std::string, Env, Env);
        
        Exp clone();
        std::string toString();
};

class MagnitudeExp : public Expression {
    private:
        Exp exp;
    public:
        MagnitudeExp(Exp e) : exp(e) {}
        ~MagnitudeExp() { delete exp; }
        Exp clone() { return new MagnitudeExp(exp->clone()); }

        Val valueOf(Env);
        Val derivativeOf(std::string, Env, Env);

        std::string toString();
};

class MapExp : public Expression {
    private:
        Exp func;
        Exp list;
    public:
        MapExp(Exp l, Exp f) : func(f), list(l) {}
        ~MapExp() { delete func; delete list; }
        Exp clone() { return new MapExp(list->clone(), func->clone()); }

        Val valueOf(Env);
        Val derivativeOf(std::string, Env, Env);
        
        std::string toString();
};

class NormExp : public Expression {
    private:
        Exp exp;
    public:
        NormExp(Exp e) : exp(e) {}
        ~NormExp() { delete exp; }
        Exp clone() { return new NormExp(exp->clone()); }

        Val valueOf(Env);
        //Val derivativeOf(std::string, Env, Env);

        std::string toString();
};

// Bool -> Bool expression that negates booleans
class NotExp : public Expression {
    private:
        Exp exp;
    public:
        NotExp(Exp e) { exp = e; }
        ~NotExp() { delete exp; }

        Val valueOf(Env);
        
        Exp clone() { return new NotExp(exp->clone()); }
        std::string toString();
};

class SequenceExp : public Expression {
    private:
        LinkedList<Exp> *seq;
    public:
        SequenceExp() : seq(new LinkedList<Exp>) {}
        SequenceExp(LinkedList<Exp>* es) : seq(es) {}
        ~SequenceExp() { while (!seq->isEmpty()) delete seq->remove(0); delete seq; }

        LinkedList<Exp>* getSeq() { return seq; }

        Val valueOf(Env);
        
        Exp clone();
        std::string toString();
};

// Expression for redefining values in a store
class SetExp : public Expression {
    private:
        Exp *tgts;
        Exp *exps;
    public:
        SetExp(Exp*, Exp*);
        ~SetExp() {
            for (int i = 0; tgts[i]; i++) { delete tgts[i]; delete exps[i]; }
            delete[] tgts; delete[] exps;
        }

        Val valueOf(Env);
        Val derivativeOf(std::string, Env, Env);
        
        Exp clone();
        std::string toString();
};

class ThunkExp : public Expression {
    private:
        Exp exp;
    public:
        ThunkExp(Exp e) : exp(e) {}
        ~ThunkExp() { delete exp; }

        Val valueOf(Env env) { return new Thunk(exp->clone(), env->clone()); }

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

        Val valueOf(Env) { val->add_ref(); return val; }

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

        Val valueOf(Env);
        Val derivativeOf(std::string, Env, Env);

        Exp clone() { return new WhileExp(cond->clone(), body->clone(), alwaysEnter); }
        std::string toString();
};

#endif
