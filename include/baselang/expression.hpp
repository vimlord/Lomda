#ifndef _BASELANG_EXPRESSION_HPP_
#define _BASELANG_EXPRESSION_HPP_

#include "baselang/value.hpp"
#include "baselang/environment.hpp"
#include "stringable.hpp"

// Error functions
void throw_warning(std::string form, std::string mssg);
void throw_err(std::string form, std::string mssg);
void throw_debug(std::string form, std::string mssg);

#include <unordered_map>

// Interface for expressions.
class Expression : public Stringable {
    public:
        /**
         * The default behavior on deletion is to do nothing.
         */
        virtual ~Expression() {}

        /**
         * Given an environment, compute the value of the expression.
         * Should the expression be invalid, the function will return NULL.
         */
        virtual Value* valueOf(Env) { return NULL; }
        
        /**
         * Computes the derivative of an expression. The default is to
         * fail to compute (return NULL).
         */
        virtual Value* derivativeOf(std::string, Env, Env) {
            throw_err("calculus", "expression '" + toString() + "' is non-differentiable");
            return NULL;
        }
        
        /**
         * Creates a deep copy of the expression.
         */
        virtual Expression* clone() = 0;
        

        /**
         * Performs optimization operations on an expression.
         * default: do nothing
         */
        virtual Expression* optimize() { return this; };
        /**
         * Optimization operation that performs 'constant propagation', in which
         * constant variables are replaced with constants to reduce computation
         * time.
         * default: do nothing
         */
        virtual Expression* opt_const_prop(
                std::unordered_map<std::string, Expression*>&
        ) { return this; };

};
typedef Expression* Exp;

void throw_type_err(Expression *exp, std::string type);

#endif
