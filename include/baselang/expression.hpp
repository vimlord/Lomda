#ifndef _BASELANG_EXPRESSION_HPP_
#define _BASELANG_EXPRESSION_HPP_

#include "baselang/value.hpp"
#include "baselang/environment.hpp"
#include "stringable.hpp"

// Error functions
void throw_warning(std::string form, std::string mssg);
void throw_err(std::string form, std::string mssg);
void throw_debug(std::string form, std::string mssg);

// Interface for expressions.
class Expression : public Stringable {
    public:
        virtual ~Expression() {}

        /**
         * Given an environment, compute the value of the expression.
         * Should the expression be invalid, the function will return NULL.
         */
        virtual Value* valueOf(Env) { return NULL; }

        virtual Value* derivativeOf(std::string, Env, Env) {
            throw_err("calculus", "expression '" + toString() + "' is non-differentiable");
            return NULL;
        }
        
        /**
         * Creates a deep copy of the expression.
         */
        virtual Expression* clone() = 0;
};
typedef Expression* Exp;

void throw_type_err(Expression *exp, std::string type);

#endif
