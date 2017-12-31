#ifndef _EXPRESSIONS_OPERATOR_HPP_
#define _EXPRESSIONS_OPERATOR_HPP_

#include "expressions/derivative.hpp"

class OperatorExp : public Differentiable {
    protected:
        Expression *left;
        Expression *right;
    public:
        OperatorExp(Exp, Exp);
        ~OperatorExp();
        Value* valueOf(Env);
        
        virtual Value* op(Value*, Value*) = 0;
};

class AndExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;
        Value* op(Value*, Value*);
        
        Value* derivativeOf(std::string, Env, Env);

        Exp clone() { return new AndExp(left->clone(), right->clone()); }
        std::string toString();
};

class DiffExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Value* op(Value*, Value*);
        Value* derivativeOf(std::string, Env, Env);
        
        Exp clone() { return new DiffExp(left->clone(), right->clone()); }
        std::string toString();
};

class DivExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Value* op(Value*, Value*);
        Value* derivativeOf(std::string, Env, Env);
        
        Exp clone() { return new DivExp(left->clone(), right->clone()); }
        std::string toString();
};

enum CompOp {
    EQ, NEQ, GT, LT, LEQ, GEQ
};
class CompareExp : public OperatorExp {
    private:
        CompOp operation;
    public:
        CompareExp(Exp l, Expression *r, CompOp op) : OperatorExp(l, r) {
            operation = op;
        }
        Value* op(Value*, Value*);

        Value* derivativeOf(std::string, Env, Env);
        
        Exp clone() { return new CompareExp(left->clone(), right->clone(), operation); }
        std::string toString();
};

// Expression for multiplying numbers.
class MultExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Value* op(Value*, Value*);
        Value* derivativeOf(std::string, Env, Env);
        
        Exp clone() { return new MultExp(left->clone(), right->clone()); }
        std::string toString();
};

// Expression for adding numbers.
class SumExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Value* op(Value*, Value*);
        Value* derivativeOf(std::string, Env, Env);
        
        Exp clone() { return new SumExp(left->clone(), right->clone()); }
        std::string toString();
};

#endif
