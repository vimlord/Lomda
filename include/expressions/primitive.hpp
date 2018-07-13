#ifndef _EXPRESSIONS_PRIMITIVE_HPP_
#define _EXPRESSIONS_PRIMITIVE_HPP_

#include "baselang/expression.hpp"
#include "value.hpp"

#include "types.hpp"

#include <initializer_list>
#include <utility>

class PrimitiveExp : public Expression {
    public:
        Exp optimize();
};

/**
 * Denotes an ADT.
 */
class AdtExp : public Expression {
    private:
        std::string name, kind;
        Exp *args;
    public:
        AdtExp(std::string n, std::string k, Exp *xs)
        : name(n), kind(k), args(xs) {}
        ~AdtExp() {
            for (int i = 0; args[i]; i++)
                delete args[i];
            delete[] args;
        }

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);
        
        Exp clone();
        std::string toString();

        bool postprocessor(Trie<bool> *vars);
};

/**
 * Denotes a dictionary.
 */
class DictExp : public Expression {
    private:
        LinkedList<std::string> *keys;
        LinkedList<Exp> *vals;
    public:
        DictExp() : keys(new LinkedList<std::string>), vals(new LinkedList<Exp>) {}
        DictExp(LinkedList<std::string> *ks, LinkedList<Exp> *vs) : keys(ks), vals(vs) {}
        DictExp(std::initializer_list<std::pair<std::string, Exp>>);
        
        ~DictExp() {
            delete keys;
            while (!vals->isEmpty()) delete vals->remove(0);
            delete vals;
        }

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);
        
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
};

/**
 * Denotes false.
 */
class FalseExp : public PrimitiveExp {
    public:
        FalseExp() {}
        Val evaluate(Env);
        Type* typeOf(Tenv tenv);
        
        Exp clone() { return new FalseExp(); }
        std::string toString() { return "false"; }

};

/**
 * Denotes a member of the set of 32 bit unsigned integers.
 */
class IntExp : public PrimitiveExp {
    private:
        int val;
    public:
        IntExp(int = 0);
        Val evaluate(Env);
        Type* typeOf(Tenv tenv);
        Val derivativeOf(std::string, Env, Env);

        int get() { return val; }
        
        Exp clone() { return new IntExp(val); }
        std::string toString();
};

/**
 * Expression that yields closures (lambdas)
 */
class LambdaExp : public Expression {
    private:
        std::string *xs;
        Exp exp;
    public:
        LambdaExp(std::string*, Exp);
        ~LambdaExp() { delete[] xs; delete exp; }
        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);

        std::string *getXs() { return xs; }
        
        Exp clone();
        std::string toString();

        bool postprocessor(Trie<bool> *vars);

        Exp optimize() { exp = exp->optimize(); return this; }
};

/**
 * Expression that represents a list of expressions.
 */
class ListExp : public Expression, public ArrayList<Exp> {
    public:
        ListExp();
        ListExp(Exp*);
        ListExp(List<Exp>* l);

        ~ListExp();

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);
        
        Exp clone();
        std::string toString();

        bool postprocessor(Trie<bool> *vars) {
            auto res = true;
            for (int i = 0; i < size(); i++) {
                res = get(i)->postprocessor(vars);
            }
            return res;
        }
        
        Exp optimize();
};

/**
 * Expression that represents a 32-bit floating point real number.
 */
class RealExp : public PrimitiveExp {
    private:
        float val;
    public:
        RealExp(float = 0);
        Val evaluate(Env);
        Type* typeOf(Tenv tenv);
        Val derivativeOf(std::string, Env, Env);
        
        Exp clone() { return new RealExp(val); }
        std::string toString();

};

/**
 * Denotes a string within the language.
 */
class StringExp : public PrimitiveExp {
    private:
        std::string val;
    public:
        StringExp(std::string s) : val(s) {}

        Val evaluate(Env) { return new StringVal(val); }
        Type* typeOf(Tenv) { return new StringType; }

        Exp clone() { return new StringExp(val); }
        std::string toString();

};

/**
 * Denotes true.
 */
class TrueExp : public PrimitiveExp {
    public:
        TrueExp() {}
        Val evaluate(Env);
        Type* typeOf(Tenv tenv);
        
        Exp clone() { return new TrueExp(); }
        std::string toString() { return "true"; }

};

/**
 * Denotes a tuple, which consists of two expressions.
 */
class TupleExp : public Expression {
    private:
        Exp left;
        Exp right;
    public:
        TupleExp(Exp l, Exp r) : left(l), right(r) {}
        ~TupleExp() { delete left; delete right; }

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv tenv);
        std::string toString();

        Exp clone() { return new TupleExp(left->clone(), right->clone()); }

        bool postprocessor(Trie<bool> *vars) {
            return left->postprocessor(vars) && right->postprocessor(vars);
        }

        Exp optimize() { left = left->optimize(); right = right->optimize(); return this; }
};

// Get the value of a variable
class VarExp : public Expression {
    private:
        std::string id;
    public:
        VarExp(std::string s) : id(s) {}

        Val evaluate(Env env);
        Type* typeOf(Tenv tenv);
        
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
};

class VoidExp : public PrimitiveExp {
    public:
        VoidExp() {}

        Val evaluate(Env) { return new VoidVal; }
        Type* typeOf(Tenv) { return new VoidType; }

        std::string toString() { return "void"; }

        Exp clone() { return new VoidExp; }
};

#endif
