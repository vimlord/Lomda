#ifndef _ENVIRONMENT_HPP_
#define _ENVIRONMENT_HPP_

#include "baselang/environment.hpp"

#include "value.hpp"

class EmptyEnv : public Environment<Value> {
    public:
        EmptyEnv();
        ~EmptyEnv();

        /**
         * Attempts to (and fails) to find the value of the variable.
         *
         * @param x The variable to search for.
         *
         * @return The value of the expression (almost certain to be NULL).
         */
        Value* apply(std::string x);

        Environment* clone();

        std::string toString();
};

class ExtendEnv : public Environment<Value> {
    private:
        // Identifier -> Value mapping
        std::string id;
        Value* ref;
    public:
        ExtendEnv(std::string, Value*, Environment* = NULL);
        ~ExtendEnv();
        
        /**
         * Searches this environment for the value of the given variable.
         *
         * Given the name of a variable, attempts to compute the value of a
         * variable. If the top layer contains the value, then return the
         * stored value. Otherwise, continue the search in the next layer.
         *
         * @param x The variable to search for.
         *
         * @return The value of the variable if it is defined, otherwise NULL.
         */
        Val apply(std::string x);
        int set(std::string s, Val v) {if (s == id) { ref->rem_ref(); ref = v; return 0;} else return subenv->set(s, v); }

        std::string topId() { return id; }
        Value* topVal() { return ref; }

        Environment* clone();

        std::string toString();
};

#endif
