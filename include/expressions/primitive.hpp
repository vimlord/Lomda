#ifndef _EXPRESSIONS_PRIMITIVE_HPP_
#define _EXPRESSIONS_PRIMITIVE_HPP_

#include "baselang/expression.hpp"
#include "value.hpp"

#include "types.hpp"

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

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        
        Exp clone();
        std::string toString();

        bool postprocessor(Trie<bool> *vars) {
            auto it = vals->iterator();
            auto res = true;
            while (res && it->hasNext())
                res = it->next()->postprocessor(vars);
            delete it;
            return res;
        }

        Exp optimize();
        int opt_var_usage(std::string);
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&);
};

// Generates false
class FalseExp : public Expression {
    public:
        FalseExp() {}
        Val evaluate(Env);
        Type* typeOf(Tenv tenv) { return new BoolType; }
        
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
        Val evaluate(Env);
        Type* typeOf(Tenv tenv) { return new IntType; }
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
        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);

        std::string *getXs() { return xs; }
        
        Exp clone();
        std::string toString();

        bool postprocessor(Trie<bool> *vars);

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

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        
        Exp clone();
        std::string toString();

        List<Exp>* getList() { return list; }

        bool postprocessor(Trie<bool> *vars) {
            auto it = list->iterator();
            auto res = true;
            while (res && it->hasNext())
                res = it->next()->postprocessor(vars);
            delete it;
            return res;
        }
        
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
        Val evaluate(Env);
        Type* typeOf(Tenv tenv) { return new RealType; }
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

        Val evaluate(Env) { return new StringVal(val); }

        Exp clone() { return new StringExp(val); }
        std::string toString();

        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&) { return this; }
        int opt_var_usage(std::string) { return 0; }
};

// Generates true
class TrueExp : public Expression {
    public:
        TrueExp() {}
        Val evaluate(Env);
        Type* typeOf(Tenv tenv) { return new BoolType; }
        
        Exp clone() { return new TrueExp(); }
        std::string toString() { return "true"; }

        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&) { return this; }
        int opt_var_usage(std::string) { return 0; }
};

class TupleExp : public Expression {
    private:
        Exp left;
        Exp right;
    public:
        TupleExp(Exp l, Exp r) : left(l), right(r) {}
        ~TupleExp() { delete left; delete right; }

        Val evaluate(Env);
        Type* typeOf(Tenv tenv) { return new TupleType(left->typeOf(tenv), right->typeOf(tenv)); }
        std::string toString();

        Exp clone() { return new TupleExp(left->clone(), right->clone()); }

        bool postprocessor(Trie<bool> *vars) {
            return left->postprocessor(vars) && right->postprocessor(vars);
        }

        Exp optimize() { left = left->optimize(); right = right->optimize(); return this; }
        int opt_var_usage(std::string x) { return left->opt_var_usage(x) | right->opt_var_usage(x); }
};

// Get the value of a variable
class VarExp : public Expression {
    private:
        std::string id;
    public:
        VarExp(std::string s) : id(s) {}

        Val evaluate(Env env);
        
        Exp clone() { return new VarExp(id); }
        std::string toString();

        Val derivativeOf(std::string, Env, Env);

        bool postprocessor(Trie<bool> *vars) {
            if (vars->hasKey(id)) return true;
            throw_err("", "undefined reference to variable " + id);
            return false;
        }

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

        Val evaluate(Env) { return new VoidVal; }
        std::string toString() { return "void"; }

        Exp clone() { return new VoidExp; }
        
        Exp opt_const_prop(opt_varexp_map&, opt_varexp_map&) { return this; }
        int opt_var_usage(std::string) { return 0; }
};

#endif
