#ifndef _ENVIRONMENT_HPP_
#define _ENVIRONMENT_HPP_

#include "baselang/environment.hpp"

#include "value.hpp"

// Definition of the empty environment.
class EmptyEnv : public Environment {
    public:
        EmptyEnv();
        Value* apply(std::string);

        Environment* copy();
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
        Value* apply(std::string);

        std::string topId() { return id; }
        Value* topVal() { return ref; }

        Environment* copy();
        void destroy(int);

        std::string toString();
};

#endif
