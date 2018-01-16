#ifndef _VALUE_HPP_
#define _VALUE_HPP_

#include "baselang/value.hpp"
#include "baselang/expression.hpp"
#include "baselang/environment.hpp"

#include "linalg/matrix.hpp"

class BoolVal : public Value {
    private:
        bool val;
    public:
        BoolVal(bool = 0);
        bool get();
        std::string toString();
        BoolVal* clone() { return new BoolVal(val); }
        int set(Value*);
};

class IntVal : public Value {
    private:
        int val;
    public:
        IntVal(int = 0);
        int get();
        std::string toString();
        IntVal* clone() { return new IntVal(val); }
        int set(Value*);
};

// Represents a closure
class LambdaVal : public Value {
    private:
        std::string *xs;
        Expression *exp;
        Environment *env;
    public:
        LambdaVal(std::string*, Expression*, Environment* = NULL);
        ~LambdaVal();
        std::string toString();
        Value* apply(Value **xs, Environment *e = NULL);
        LambdaVal* clone();
        int set(Value*);

        std::string* getArgs() { return xs; }

        Expression* getBody() { return exp; }
        
        Environment* getEnv() { return env; }
        void setEnv(Environment*);
};

class ListVal : public Value {
    // Note: it may be of interest to make this a subclass of
    // some form of map value in order to generalize certain
    // behaviors.
    private:
        List<Value*> *list;
    public:
        ListVal() { list = new LinkedList<Value*>(); }
        ListVal(List<Value*> *l) : list(l) {}
        ~ListVal();

        List<Value*>* get() { return list; }
        ListVal* clone();
        int set(Value*);

        std::string toString();
};

class RealVal : public Value {
    private:
        float val;
    public:
        RealVal(float = 0);
        float get();
        int set(Value*);

        std::string toString();
        RealVal* clone() { return new RealVal(val); }
};

class StringVal : public Value {
    private:
        std::string val;
    public:
        StringVal(std::string s) : val(s) {}

        std::string get() { return val; }
        int set(Value*);

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
        ~Thunk() { delete exp; if (val) val->rem_ref(); }
        
        Val get(Env e = NULL) {
            if (!e) e = env;
            if (!val) val = exp->valueOf(env);
            val->add_ref();
            return val;
        }
        int set(Value*);

        std::string toString();
        Thunk* clone() { if (val) val->add_ref(); return new Thunk(exp->clone(), env->clone(), val); }
};

class VoidVal : public Value {
    public:
        std::string toString() { return "void"; }
        VoidVal* clone() { return new VoidVal; }
        int set(Value*) {}
};

#endif
