#ifndef _BASELANG_ENVIRONMENT_HPP_
#define _BASELANG_ENVIRONMENT_HPP_

#include "stringable.hpp"
#include "reffable.hpp"

#include "baselang/value.hpp"
#include "baselang/types.hpp"
#include "structures/list.hpp"

#include <iostream>
#include <cstddef>

// Interface for environments.
class Environment : public Stringable, public Reffable {
    protected:
        Environment* subenv;

        // The majority of environments have subenvironments
        std::unordered_map<std::string, Val> store;
    public:
        Environment(Environment *env = NULL);
        ~Environment();
        /**
         * Attempts to apply a variable name to an expression.
         *
         * @param x The variable to lookup.
         *
         * @return The value of the given variable, or NULL if it was not found.
         */
        virtual Val apply(std::string x);

        /**
         * Reassigns the value of a variable.
         *
         * @param x The name of the variable.
         * @param v The value to assign to the variable (must be of the same type).
         *
         * @return Zero if the value is successfully assigned, otherwise non-zero.
         */
        int set(std::string, Val);
        void rem(std::string);

        
        virtual void add_ref() {
            this->Reffable::add_ref();
            if (subenv) subenv->add_ref();
        }
        virtual void rem_ref() {
            Environment *e = subenv;
            this->Reffable::rem_ref();
            if (e) e->rem_ref();
        }

        Environment* clone();

        /**
         * Gathers the child environment of this environment.
         *
         * @return The child environment.
         */
        Environment* subenvironment() { return subenv; }
        std::unordered_map<std::string, Val>& get_store() { return store; }

        std::string toString();
};

// We will define Env as an environment of values
typedef Environment* Env;

#endif
