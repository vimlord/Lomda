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
        // Applies an identifier to the environment to get its Value.
        virtual Value* apply(std::string) = 0;

        virtual Environment *copy() = 0;
        virtual void destroy(int) = 0;

        Environment* subenvironment();
        
        // Environments will use stores to allow reassignment.
        //void setStore(Store*);
        //Store *getStore();

        // apply(), but for retrieving the reference of a value.
        //virtual int applyStore(std::string) = 0;

};

#endif
