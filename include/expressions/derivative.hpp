#ifndef _EXPRESSIONS_DERIVATIVE_HPP_
#define _EXPRESSIONS_DERIVATIVE_HPP_

#include "baselang/expression.hpp"
#include "structures/list.hpp"

// Struct for storing derivative information
struct denv_frame {
    std::string id;
    Expression *val;
};
typedef struct denv_frame DenvFrame;
bool operator==(const DenvFrame& lhs, const DenvFrame& rhs);

typedef LinkedList<DenvFrame>* DerivEnv;

class Differentiable : public Expression {
    public:
        /**
         * Computes the derivative of an expression.
         * env - The known variable values.
         * denv - The known derivative values.
         */
        virtual Expression* derivativeOf(DerivEnv denv) = 0;

        static Expression* applyDenv(DerivEnv, std::string);
        static int setDenv(DerivEnv, std::string, Expression*);
};


#endif
