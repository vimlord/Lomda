#ifndef _EXPRESSIONS_OPERATOR_HPP_
#define _EXPRESSIONS_OPERATOR_HPP_

#include "expressions/derivative.hpp"

class OperatorExp : public Differentiable {
    protected:
        Expression *left;
        Expression *right;
    public:
        OperatorExp(Expression*, Expression*);
        ~OperatorExp() { delete left, right; }
        Value* valueOf(Environment*);
        
        virtual Value* op(Value*, Value*) = 0;
};

class AndExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;
        Value* op(Value*, Value*);
        Expression* derivativeOf(DerivEnv) {
            throw_err("runtime", "expression '" + toString() + "' is non-differentiable");
            return NULL;
        }
        
        Expression* clone() { return new AndExp(left->clone(), right->clone()); }
        std::string toString();
};

class DiffExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Value* op(Value*, Value*);
        Expression* derivativeOf(DerivEnv denv);
        
        Expression* clone() { return new DiffExp(left->clone(), right->clone()); }
        std::string toString();
};

class EqualsExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;
        Value* op(Value*, Value*);
        Expression* derivativeOf(DerivEnv) {
            throw_err("runtime", "expression '" + toString() + "' is non-differentiable");
            return NULL;
        }
        
        Expression* clone() { return new EqualsExp(left->clone(), right->clone()); }
        std::string toString();
};

// Expression for adding numbers.
class MultExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Value* op(Value*, Value*);
        Expression* derivativeOf(DerivEnv denv);
        
        Expression* clone() { return new MultExp(left->clone(), right->clone()); }
        std::string toString();
};;

// Expression for adding numbers.
class SumExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Value* op(Value*, Value*);
        Expression* derivativeOf(DerivEnv denv);
        
        Expression* clone() { return new SumExp(left->clone(), right->clone()); }
        std::string toString();
};

#endif
