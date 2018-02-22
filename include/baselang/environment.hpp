#ifndef _BASELANG_ENVIRONMENT_HPP_
#define _BASELANG_ENVIRONMENT_HPP_

#include "stringable.hpp"
#include "reffable.hpp"

#include "baselang/value.hpp"
#include "baselang/types.hpp"
#include "structures/list.hpp"

#include <iostream>
#include <cstddef>

typedef List<Value*> Store;

// Interface for environments.
template<class T>
class Environment : public Stringable, public Reffable {
    protected:
        // The majority of environments have subenvironments
        Environment* subenv = NULL;
    public:
        /**
         * Attempts to apply a variable name to an expression.
         *
         * @param x The variable to lookup.
         *
         * @return The value of the given variable, or NULL if it was not found.
         */
        virtual T* apply(std::string x) { return NULL; }

        /**
         * Reassigns the value of a variable.
         *
         * @param x The name of the variable.
         * @param v The value to assign to the variable (must be of the same type).
         *
         * @return Zero if the value is successfully assigned, otherwise non-zero.
         */
        virtual int set(std::string x, T* v) { return 1; }
        
        virtual void add_ref() {
            this->Reffable::add_ref();
            if (subenv) subenv->add_ref();
        }
        virtual void rem_ref() {
            Environment *e = subenv;
            this->Reffable::rem_ref();
            if (e) e->rem_ref();
        }

        virtual Environment<T>* clone() = 0;
        
        /**
         * Gathers the child environment of this environment.
         *
         * @return The child environment.
         */
        Environment<T>* subenvironment() { return subenv; }
};

// We will define Env as an environment of values
typedef Environment<Value>* Env;
typedef Environment<Type>* Tenv;

#endif
