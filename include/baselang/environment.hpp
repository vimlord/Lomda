#ifndef _BASELANG_ENVIRONMENT_HPP_
#define _BASELANG_ENVIRONMENT_HPP_

#include "stringable.hpp"
#include "reffable.hpp"

#include "baselang/value.hpp"
#include "structures/list.hpp"


#include <iostream>
#include <cstddef>

typedef List<Value*> Store;

// Interface for environments.
class Environment : public Stringable, public Reffable {
    protected:
        // The majority of environments have subenvironments
        Environment* subenv = NULL;
    public:
        // Applies an identifier to the environment to get its Value.
        virtual Val apply(std::string) { return NULL; }
        virtual int set(std::string, Val) { return 1; }

        virtual void add_ref() {
            this->Reffable::add_ref();
            if (subenv) subenv->add_ref();
        }
        virtual void rem_ref() {
            this->Reffable::rem_ref();
            if (subenv) subenv->rem_ref();
        }

        virtual Environment* clone() = 0;

        Environment* subenvironment();
};
typedef Environment* Env;

#endif
