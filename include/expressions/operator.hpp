#ifndef _EXPRESSIONS_OPERATOR_HPP_
#define _EXPRESSIONS_OPERATOR_HPP_

#include "baselang/expression.hpp"

/**
 * A top-level operator expression that relies on the use of two expressions.
 */
class UnaryOperatorExp : public Expression {
    protected:
        Exp exp;
    public:
        UnaryOperatorExp(Exp e) : exp(e) {}
        ~UnaryOperatorExp() { delete exp; }

        Val evaluate(Env env);
        virtual Val op(Val) = 0;

        virtual Exp optimize() { exp = exp->optimize(); return this; }
        bool postprocessor(Trie<bool> *vars);
};

class OperatorExp : public Expression {
    protected:
        Expression *left;
        Expression *right;
    public:
        OperatorExp(Exp, Exp);
        ~OperatorExp();
        Val evaluate(Env);
        
        virtual Val op(Val, Val) = 0;

        bool postprocessor(Trie<bool> *vars)
                { return left->postprocessor(vars) && right->postprocessor(vars); }

        Exp getLeft() { return left; }
        Exp getRight() { return right; }

        virtual Exp optimize();
};

/**
 * Computes the boolean AND operation.
 */
class AndExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;
        Val op(Val, Val);
        
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);

        Exp clone() { return new AndExp(left->clone(), right->clone()); }
        std::string toString();
};

/**
 * Computes the boolean OR operation.
 */
class OrExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;
        Val op(Val, Val);
        
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);

        Exp clone() { return new OrExp(left->clone(), right->clone()); }
        std::string toString();
};

/**
 * Performs the subtraction (difference) operation on two expressions.
 */
class DiffExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Val op(Val, Val);
        Val derivativeOf(std::string, Env, Env);
        Exp symb_diff(std::string);
        Type* typeOf(Tenv);
        
        Exp clone() { return new DiffExp(left->clone(), right->clone()); }
        std::string toString();
};

class DivExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Val op(Val, Val);
        Val derivativeOf(std::string, Env, Env);
        Exp symb_diff(std::string);

        Type* typeOf(Tenv);
        
        Exp clone() { return new DivExp(left->clone(), right->clone()); }
        std::string toString();

        Exp optimize();
};

/**
 * Denotes types of comparison operations that can be used.
 */
enum CompOp {
    EQ, NEQ, GT, LT, LEQ, GEQ
};

/**
 * Performs a comparison between two expressions.
 */
class CompareExp : public OperatorExp {
    private:
        CompOp operation;
    public:
        CompareExp(Exp l, Expression *r, CompOp op) : OperatorExp(l, r) {
            operation = op;
        }
        Val op(Val, Val);

        Type* typeOf(Tenv);
        
        Exp clone() { return new CompareExp(left->clone(), right->clone(), operation); }
        std::string toString();
};

/**
 * Performs exponentiation between two expressions.
 */
class ExponentExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Val op(Val, Val);
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);

        Exp clone() { return new ExponentExp(left->clone(), right->clone()); }
        std::string toString();
};

/**
 * Expression for multiplying numbers.
 */
class MultExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Val op(Val, Val);
        Val derivativeOf(std::string, Env, Env);
        Exp symb_diff(std::string);

        Type* typeOf(Tenv);
        
        Exp clone() { return new MultExp(left->clone(), right->clone()); }
        std::string toString();

        Exp optimize();
};

/**
 * Expression for adding numbers.
 */
class SumExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Val op(Val, Val);
        Val derivativeOf(std::string, Env, Env);
        Exp symb_diff(std::string);

        Type* typeOf(Tenv);
        
        Exp clone() { return new SumExp(left->clone(), right->clone()); }
        std::string toString();

        Exp optimize();
};

/**
 * Expression for modulus.
 */
class ModulusExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Val op(Val, Val);
        Val derivativeOf(std::string, Env, Env);
        Exp symb_diff(std::string);

        Type* typeOf(Tenv);
        
        Exp clone() { return new ModulusExp(left->clone(), right->clone()); }
        std::string toString();
};

#endif
