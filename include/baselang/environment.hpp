#ifndef _BASELANG_ENVIRONMENT_HPP_
#define _BASELANG_ENVIRONMENT_HPP_

#include "baselang/value.hpp"
#include "stringable.hpp"

#include "structures/list.hpp"

#include <iostream>
#include <cstddef>

typedef List<Value*> Store;

// Interface for environments.
class Environment : public Stringable {
    protected:
        // The majority of environments have subenvironments
        Environment* subenv = NULL;
    public:
        virtual ~Environment() {}

        // Applies an identifier to the environment to get its Value.
        virtual Value* apply(std::string) = 0;

        virtual Environment* clone() = 0;

        Environment* subenvironment();
};

#endif
