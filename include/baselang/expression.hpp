#ifndef _BASELANG_EXPRESSION_HPP_
#define _BASELANG_EXPRESSION_HPP_

#include "baselang/value.hpp"
#include "baselang/environment.hpp"
#include "baselang/types.hpp"
#include "stringable.hpp"

#include "structures/trie.hpp"

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
         *
         * @param env The environment under which the expression is to be computed.
         *
         * @return The outcome of the computation, or NULL if the expression
                    is non-computable under the given environment.
         */
        virtual Val evaluate(Env env) {
            throw_err("implementation", "expression '" + toString() + "' cannot be computed because it is not implemented");
            return NULL;
        }

        virtual Type* typeOf(Tenv tenv) {
            throw_err("type", "type of expression '" + toString() + "' cannot be computed because it is not implemented");
            return NULL;
        }
        
        /**
         * Computes the derivative of an expression. The default is to
         * fail to compute (return NULL).
         *
         * @param x The variable name under which differentiation is computed.
         * @param env The environment under which to compute.
         * @param denv The derivative of the expressions in the environment, assuming they exist.
         */
        virtual Val derivativeOf(std::string x, Env env, Env denv) {
            throw_err("calculus", "expression '" + toString() + "' is non-differentiable");
            return NULL;
        }
        virtual Expression* symb_diff(std::string x);

        virtual bool postprocessor(Trie<bool> *vars = new Trie<bool>) { return true; };
        
        /**
         * Creates a deep copy of the expression.
         *
         * @return A clone of the expression.
         */
        virtual Expression* clone() = 0;
        
        /**
         * Performs optimization operations on an expression.
         *
         * In order to perform optimization, a brand new expression will be
         * created if necessary. It may also be the case that the expression
         * will modify the fields of the expression. The best practice is that
         * one should reassign to any variable that is optimized. Please note
         * that the replacement expression may not be correct.
         *
         * @return An expression that is at least as efficient as the original.
         */
        virtual Expression* optimize() { return this; };
};
typedef Expression* Exp;

typedef std::unordered_map<std::string, Exp> opt_varexp_map;

/**
 * Displays an error message indicating a type error.
 *
 * @param exp The expression that fails to match the type.
 * @param type The type that was requested.
 */
void throw_type_err(Expression *exp, std::string type);

#endif
