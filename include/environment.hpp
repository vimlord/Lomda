#ifndef _ENVIRONMENT_HPP_
#define _ENVIRONMENT_HPP_

#include "baselang/environment.hpp"

#include "value.hpp"

// Definition of the empty environment.
class EmptyEnv : public Environment {
    public:
        EmptyEnv();
        ~EmptyEnv();
        Value* apply(std::string);

        Environment* clone();
        void destroy(int);

        std::string toString();
};

// Definition of an extending environment.
class ExtendEnv : public Environment {
    private:
        // Identifier -> Value mapping
        std::string id;
        Value* ref;
    public:
        ExtendEnv(std::string, Value*, Environment* = NULL);
        ~ExtendEnv();

        Val apply(std::string);
        int set(std::string s, Val v) {if (s == id) { ref->rem_ref(); ref = v; return 0;} else return subenv->set(s, v); }

        std::string topId() { return id; }
        Value* topVal() { return ref; }

        Environment* clone();
        void destroy(int);

        std::string toString();
};

#endif
