#ifndef _VALUE_HPP_
#define _VALUE_HPP_

#include "baselang/value.hpp"
#include "baselang/expression.hpp"
#include "baselang/environment.hpp"

#include "structures/hashmap.hpp"

bool is_zero_val(Val e);

template<typename T>
inline bool isVal(const Val t) {
    return t && dynamic_cast<const T*>(t) != nullptr;
}

/**
 * An Algebraic Datatype (ADT)
 */
class AdtVal : public Value {
    private:
        // Typing identifiers used to properly identify the ADT
        std::string type; // The global type of ADT.
        std::string kind; // The specific kind of ADT.

        Val *args; // The parameters given to create the ADT.
    public:
        AdtVal(std::string t, std::string k, Val *xs)
        : type(t), kind(k), args(xs) {}
        ~AdtVal();

        int set(Val);

        // Getters
        std::string getType() { return type; }
        std::string getKind() { return kind; }
        Val* getArgs() { return args; }

        std::string toString();
        AdtVal* clone();
};

/**
 * A boolean value.
 */
class BoolVal : public Value {
    private:
        bool val;
    public:
        BoolVal(bool = 0);
        bool get();
        std::string toString();
        BoolVal* clone() { return new BoolVal(val); }
        int set(Val);
};

/**
 * Defines a dictionary of values.
 */
class DictVal : public Value, public HashMap<std::string, Val> {
    public:
        using HashMap::HashMap;
        DictVal(std::initializer_list<std::pair<std::string, Val>>);

        ~DictVal();

        DictVal* clone();
        int set(Val);
        using HashMap::set;

        std::string toString();
};

class IntVal : public Value {
    private:
        int val;
    public:
        IntVal(int = 0);
        int get();
        std::string toString();
        IntVal* clone() { return new IntVal(val); }
        int set(Val);
};

/**
 * Represents a applicable lambda.
 */
class LambdaVal : public Value {
    private:
        std::string *xs;
        Expression *exp;
        Env env;
    public:
        LambdaVal(std::string*, Exp, Env = NULL);
        ~LambdaVal();
        std::string toString();
        Val apply(Value **xs, Env e = NULL);
        LambdaVal* clone();
        int set(Val);

        std::string* getArgs() { return xs; }

        Expression* getBody() { return exp; }
        
        Env getEnv() { return env; }
        void setEnv(Env);
};

/**
 * Represents a list of values.
 */
class ListVal : public Value, public ArrayList<Val> {
    public:
        using ArrayList::ArrayList;
        ~ListVal();

        ListVal* clone();
        int set(Val);
        using ArrayList<Val>::set;

        std::string toString();
};

class RealVal : public Value {
    private:
        float val;
    public:
        RealVal(float = 0);
        float get();
        int set(Val);

        std::string toString();
        RealVal* clone() { return new RealVal(val); }
};

class StringVal : public Value {
    private:
        std::string val;
    public:
        StringVal(std::string s) : val(s) {}

        std::string get() { return val; }
        int set(Val);

        std::string toString();
        StringVal* clone() { return new StringVal(val); }
};

/**
 * An uncomputed value that can be invoked at any time to yield an evaluated
 * value. This may be used in cases where we wish to lazily evaluate an
 * expression to avoid unnecessary computations.
 */
class Thunk : public Value {
    private:
        Exp exp;
        Env env;
        Val val;
    public:
        Thunk(Exp ex, Env en, Val v = NULL) : exp(ex), env(en), val(v) {}
        ~Thunk() { delete exp; if (val) val->rem_ref(); if (env) env->rem_ref(); }
        
        Val get(Env e = NULL);
        int set(Val);

        std::string toString();
        Thunk* clone() { if (val) val->add_ref(); return new Thunk(exp->clone(), env->clone(), val); }
};

class TupleVal : public Value {
    private:
        Val left;
        Val right;
    public:
        TupleVal(Val l, Val r) : left(l), right(r) {}
        ~TupleVal() { left->rem_ref(); right->rem_ref(); }

        Val& getLeft() { return left; }
        Val& getRight() { return right; }
        int set(Val);
        
        std::string toString();
        TupleVal* clone() { left->add_ref(); right->add_ref(); return new TupleVal(left, right); }
};

/**
 * Defines the void value, which represents nothing.
 */
class VoidVal : public Value {
    public:
        std::string toString() { return "void"; }
        VoidVal* clone() { return new VoidVal; }
        int set(Val v) { return isVal<VoidVal>(v); }
};

#endif
