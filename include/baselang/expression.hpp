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
         *
         * @param env The environment under which the expression is to be computed.
         *
         * @return The outcome of the computation, or NULL if the expression
                    is non-computable under the given environment.
         */
        virtual Val valueOf(Env env) { return NULL; }
        
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

        /**
         * Performs constant propagation on an expression.
         *
         * Optimization operation that performs 'constant propagation', in which
         * constant variables are replaced with constants to reduce computation
         * time. This operation is used in LetExp::optimize() in order to reduce
         * the need for a given LetExp in an syntax tree. The default behavior is
         * to assume that the variables cannot be propagated any further.
         *
         * @param vals A collection of values to propagate through the tree.
         * @param ends A collection of values to reassign following propagation.

         * @return An expression modified with the new constants.
         */
        virtual Expression* opt_const_prop(
                std::unordered_map<std::string, Expression*>& vals,
                std::unordered_map<std::string, Expression*>& ends
        ) { for (auto x : vals) { delete vals[x.first]; vals.erase(x.first); } return this; };

        /**
         * Returns combined variable usage, in form 00...00rw (ex: 3 means reads and writes).
         *
         * @param x The variable to find usages of.
         *
         * @return The usage of the variable.
         */
        virtual int opt_var_usage(std::string x) { return 3; }
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
