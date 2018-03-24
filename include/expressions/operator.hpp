#ifndef _EXPRESSIONS_OPERATOR_HPP_
#define _EXPRESSIONS_OPERATOR_HPP_

#include "baselang/expression.hpp"

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

        Exp optimize();
        Exp opt_const_prop(std::unordered_map<std::string, Exp>&, opt_varexp_map&);
        int opt_var_usage(std::string x) { return left->opt_var_usage(x) | right->opt_var_usage(x); }
};

class AndExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;
        Val op(Val, Val);
        
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);

        Exp clone() { return new AndExp(left->clone(), right->clone()); }
        std::string toString();
};

class OrExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;
        Val op(Val, Val);
        
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);

        Exp clone() { return new OrExp(left->clone(), right->clone()); }
        std::string toString();
};

class DiffExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Val op(Val, Val);
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);
        
        Exp clone() { return new DiffExp(left->clone(), right->clone()); }
        std::string toString();
};

class DivExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Val op(Val, Val);
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);
        
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
        Val op(Val, Val);

        Val derivativeOf(std::string, Env, Env);
        
        Exp clone() { return new CompareExp(left->clone(), right->clone(), operation); }
        std::string toString();
};

class ExponentExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Val op(Val, Val);
        Val derivativeOf(std::string, Env, Env);

        Exp clone() { return new ExponentExp(left->clone(), right->clone()); }
        std::string toString();
};

// Expression for multiplying numbers.
class MultExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Val op(Val, Val);
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);
        
        Exp clone() { return new MultExp(left->clone(), right->clone()); }
        std::string toString();
};

// Expression for adding numbers.
class SumExp : public OperatorExp {
    public:
        using OperatorExp::OperatorExp;

        Val op(Val, Val);
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);
        
        Exp clone() { return new SumExp(left->clone(), right->clone()); }
        std::string toString();
};

#endif
