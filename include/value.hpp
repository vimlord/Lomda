#ifndef _VALUE_HPP_
#define _VALUE_HPP_

#include "baselang/value.hpp"
#include "baselang/expression.hpp"
#include "baselang/environment.hpp"

bool is_zero_val(Val e);

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

class DictVal : public Value {
    private: 
        LinkedList<std::string> *keys;
        LinkedList<Val> *vals;
    public:
        DictVal() {
            keys = new LinkedList<std::string>;
            vals = new LinkedList<Val>;
        }
        DictVal(LinkedList<std::string> *ks, LinkedList<Val> *vs) {
            keys = ks;
            vals = vs;
        }
        ~DictVal() {
            delete keys;
            while (!vals->isEmpty()) vals->remove(0)->rem_ref();
            delete vals;
        }

        LinkedList<std::string>* getKeys() { return keys; }
        LinkedList<Val>* getVals() { return vals; }

        Val apply(std::string);

        std::string toString();
        DictVal* clone();
        int set(Val);
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

// Represents a closure
class LambdaVal : public Value {
    private:
        std::string *xs;
        Expression *exp;
        Env env;
    public:
        LambdaVal(std::string*, Expression*, Env = NULL);
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

class ListVal : public Value {
    // Note: it may be of interest to make this a subclass of
    // some form of map value in order to generalize certain
    // behaviors.
    private:
        ArrayList<Val> *list;
    public:
        ListVal() { list = new ArrayList<Val>(); }
        ListVal(ArrayList<Val> *l) : list(l) {}
        ~ListVal();

        ArrayList<Val>* get() { return list; }
        ListVal* clone();
        int set(Val);

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

class VoidVal : public Value {
    public:
        std::string toString() { return "void"; }
        VoidVal* clone() { return new VoidVal; }
        int set(Val v) { return typeid(*v) == typeid(VoidVal); }
};

#endif
