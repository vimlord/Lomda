#ifndef _EXPRESSION_HPP
#define _EXPRESSION_HPP

#include "baselang/expression.hpp"

#include "expressions/operator.hpp"
#include "expressions/primitive.hpp"
#include "expressions/list.hpp"
#include "expressions/stdlib.hpp"

#include <map>

template<typename T>
inline bool isExp(const Exp t) {
    return t && dynamic_cast<const T*>(t) != nullptr;
}

/**
 * Defines an expression that declares ADTs.
 */
class AdtDeclarationExp : public Expression {
    private:
        // The name of the ADT class to define.
        std::string name;
        // The name of each definition 
        std::string *ids;
        // The argument types to each kind
        Type* **argss;
        
        Exp body;
    public:
        AdtDeclarationExp(std::string nm, std::string* is, Type*** ass, Exp e)
        : name(nm), ids(is), argss(ass), body(e) {}
        ~AdtDeclarationExp();
        
        Val evaluate(Env);
        Type* typeOf(Tenv);

        bool postprocessor(Trie<bool>*);

        Exp clone();
        std::string toString();
};

/**
 * Switch-case expression for extracting ADTs.
 */
class SwitchExp : public Expression {
    private:
        // The object to test
        Exp adt;
        
        // The different categories
        std::string *names;

        // The given names
        std::string **idss;
        
        // The expressions to evaluate on successful matching
        Exp *bodies;

    public:
        SwitchExp(Exp a, std::string *nms, std::string **xs, Exp *ys)
        : adt(a), names(nms), idss(xs), bodies(ys) {}
        ~SwitchExp();

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);

        bool postprocessor(Trie<bool>*);

        Exp clone();
        std::string toString();
        
        Exp optimize();
};

/**
 * Calling functions; {a->b, a} -> b
 */
class ApplyExp : public Expression {
    private:
        Exp op;
        Exp *args;
    public:
        ApplyExp(Exp, Exp*);
        ~ApplyExp();

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);

        bool postprocessor(Trie<bool> *vars);
        
        Exp clone();
        std::string toString();
};

/**
 * Casting values; {type t, a} -> t
 */
class CastExp : public Expression {
    private:
        Type *type;
        Exp exp;
    public:
        CastExp(Type *t, Exp e) : type(t), exp(e) {}
        ~CastExp() { delete exp; delete type; }

        Val evaluate(Env);
        Type* typeOf(Tenv);

        Exp clone() { return new CastExp(type->clone(), exp->clone()); }
        std::string toString();

        Exp optimize() { exp->optimize(); return this; }
};

/**
 * Differential operator that takes the derivative of an expression.
 * This can be symbolic (implicit) or explicit depending on the context.
 */
class DerivativeExp : public Expression {
    private:
        Exp func;
        std::string var;
    public:
        DerivativeExp(Exp f, std::string s) : func(f), var(s) {}
        ~DerivativeExp() { delete func; }

        Val evaluate(Env);
        Exp symb_diff(std::string);

        Type* typeOf(Tenv);

        bool postprocessor(Trie<bool> *vars) {
            return func->postprocessor(vars);
        }

        Exp clone() { return new DerivativeExp(func->clone(), var); }
        std::string toString();
};

/**
 * Performs the higher order fold operation on an expression.
 */
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
        Type* typeOf(Tenv);

        Exp clone() { return new FoldExp(list->clone(), func->clone(), base->clone()); }
        std::string toString();
        
        bool postprocessor(Trie<bool> *vars);

        Exp optimize();
};

/**
 * Performs a for loop over a given list.
 */
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
        Type* typeOf(Tenv);

        Exp clone() { return new ForExp(id, set->clone(), body->clone()); }
        std::string toString();

        bool postprocessor(Trie<bool> *vars);

        Exp optimize();
};

/**
 * Checks whether or not an item is present in a given data structure.
 */
class HasExp : public Expression {
    private:
        Exp item;
        Exp set;
    public:
        HasExp(Exp x, Exp l) : item(x), set(l) {}
        ~HasExp() { delete item; delete set; }

        Val evaluate(Env);
        Type* typeOf(Tenv);

        std::string toString();

        bool postprocessor(Trie<bool> *vars);

        Exp optimize() {
            item = item->optimize();
            set = set->optimize();
            return this;
        }

        Exp clone() { return new HasExp(item->clone(), set->clone()); }
};

/**
 * Condition expression that chooses paths
 */
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
        Type* typeOf(Tenv);
        
        Exp clone() { return new IfExp(cond->clone(), tExp->clone(), fExp->clone()); }
        std::string toString();

        Exp optimize();
};

/**
 * Imports a module into the program. Modules can be derived from standard
 * libraries or from user created libraries.
 */
class ImportExp : public Expression {
    private:
        std::string module;
        std::string name;
        Exp exp;
    protected:
        // We will maintain a cache of previously imported modules
        // so that if a module is already loaded, it is not reloaded.
        static std::map<std::string, Val> module_cache;
    public:
        // import m
        ImportExp(std::string m, Exp e) : ImportExp(m, m, e) {}
        // import m as n
        ImportExp(std::string m, std::string n, Exp e)
                : module(m), name(n), exp(e) {
                    // Default behavior: return the expression
                    if (!e) exp = new VarExp(m);
        }

        bool postprocessor(Trie<bool> *vars) {
            if (vars->hasKey(name)) {
                throw_err("error", "redefinition of variable " + name + " is not permitted");
                return NULL;
            } else {
                vars->add(name, true);
                auto res = exp->postprocessor(vars);
                vars->remove(name);
                return res;
            }
        }

        Val evaluate(Env);
        Type* typeOf(Tenv);

        static void clear_cache();

        Exp clone() { return new ImportExp(module, name, exp->clone()); }
        std::string toString();
};

/**
 * Takes user input from stdin.
 */
class InputExp : public Expression {
    public:
        InputExp() {}
        
        Val evaluate(Env);
        Type* typeOf(Tenv) { return new StringType; }

        Exp clone() { return new InputExp; }
        std::string toString() { return "input"; }

};

/**
 * Given an expression and a string denoting the type, decide
 * whether or not the types match.
 */
class IsaExp : public Expression {
    private:
        Exp exp;
        Type *type;
    public:
        IsaExp(Exp e, Type *t) : exp(e), type(t) {}
        ~IsaExp() { delete exp; delete type; }

        Val evaluate(Env);
        Type* typeOf(Tenv);

        Exp clone() { return new IsaExp(exp->clone(), type->clone()); }
        std::string toString();

        bool postprocessor(Trie<bool> *vars);

        Exp optimize() { exp = exp->optimize(); return this; }
};

/**
 * Expression for defining variables
 */
class LetExp : public Expression {
    private:
        std::string *ids;
        Exp *exps;
        Exp body;

        // For a given index, whether or not the value should handle recursion.
        bool *rec;
    public:
        LetExp(std::string*, Exp*, Exp, bool* = NULL);
        ~LetExp() {
            for (int i = 0; exps[i]; i++) delete exps[i];
            delete[] exps;
            delete body;
            delete[] ids;
            delete[] rec;
        }

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env); 
        Type* typeOf(Tenv);
        
        Exp clone();
        std::string toString();

        bool postprocessor(Trie<bool> *vars);

        Exp optimize();
};

/**
 * For lists, determine the size. For numbers, compute the absolute value.
 */
class MagnitudeExp : public UnaryOperatorExp {
    public:
        using UnaryOperatorExp::UnaryOperatorExp;

        Exp clone() { return new MagnitudeExp(exp->clone()); }
        Type* typeOf(Tenv);

        Val op(Val);
        Val derivativeOf(std::string, Env, Env);

        std::string toString();
};

/**
 * Performs the higher order map operation on a list given a function
 * that transforms each element.
 */
class MapExp : public Expression {
    private:
        Exp func;
        Exp list;
    public:
        MapExp(Exp f, Exp l) : func(f), list(l) {}
        ~MapExp() { delete func; delete list; }
        Exp clone() { return new MapExp(func->clone(), list->clone()); }

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);
        
        std::string toString();

        bool postprocessor(Trie<bool> *vars) { return func->postprocessor(vars) && list->postprocessor(vars); }

        Exp optimize() { func = func->optimize(); list = list->optimize(); return this; }
};

/**
 * Determines th magnitude of a given expression.
 */
class NormExp : public UnaryOperatorExp {
    public:
        using UnaryOperatorExp::UnaryOperatorExp;

        Exp clone() { return new NormExp(exp->clone()); }

        Val op(Val);
        //Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);

        std::string toString();
};

/**
 * Bool -> Bool expression that negates booleans
 */
class NotExp : public UnaryOperatorExp {
    public:
        using UnaryOperatorExp::UnaryOperatorExp;

        Val op(Val);
        Type* typeOf(Tenv);
        
        Exp clone() { return new NotExp(exp->clone()); }
        std::string toString();

        Exp optimize();
};

/**
 * Executes a sequence of expressions, preserving some elenents of environment
 * in doing so.
 */
class SequenceExp : public Expression {
    private:
        LinkedList<Exp> *seq;
    public:
        SequenceExp() : seq(new LinkedList<Exp>) {}
        SequenceExp(LinkedList<Exp>* es) : seq(es) {}
        ~SequenceExp() { while (!seq->isEmpty()) delete seq->remove(0); delete seq; }

        LinkedList<Exp>* getSeq() { return seq; }

        Val evaluate(Env);
        Type* typeOf(Tenv);
        
        Exp clone();
        std::string toString();

        bool postprocessor(Trie<bool> *vars);

        Exp optimize();
};

/**
 * Expression for redefining values in a store
 */
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
        Type* typeOf(Tenv);
        
        Exp clone();
        std::string toString();

        bool postprocessor(Trie<bool> *vars);

        Exp optimize();
};

/**
 * Generates a thunk of the given expression, which will be evaluated
 * in the future. The thunk will be vulnerable to side effects.
 */
class ThunkExp : public Expression {
    private:
        Exp exp;
    public:
        ThunkExp(Exp e) : exp(e) {}
        ~ThunkExp() { delete exp; }

        Val evaluate(Env env) { env->add_ref(); return new Thunk(exp->clone(), env); }
        Val derivativeOf(std::string x, Env e, Env d) { return exp->derivativeOf(x, e, d); }
        Exp symb_diff(std::string x);

        Type* typeOf(Tenv tenv) { return exp->typeOf(tenv); }

        bool postprocessor(Trie<bool> *vars);

        Exp optimize() { exp = exp->optimize(); return this; }
        Exp clone() { return new ThunkExp(exp->clone()); }
        std::string toString();
};

/**
 * An anti-thunk expression. Evaluates to the stored value.
 */
class ValExp : public Expression {
    private:
        Val val;
    public:
        ValExp(Val v) : val(v) { val->add_ref(); }
        ~ValExp() { val->rem_ref(); }

        Val evaluate(Env) { val->add_ref(); return val; }

        Exp clone() { return new ValExp(val); }
        std::string toString();
};

/**
 * Repeats a given operation while a condition holds.
 */
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
        Type* typeOf(Tenv);

        Exp clone() { return new WhileExp(cond->clone(), body->clone(), alwaysEnter); }
        std::string toString();

        bool postprocessor(Trie<bool> *vars);

        Exp optimize();
};

#endif
