#include "expression.hpp"

#include "value.hpp"
#include "config.hpp"

#include <string>

using namespace std;


void throw_warning(string form, string mssg) {
    if (WERROR()) throw_err(form, mssg);
    else std::cerr << "\x1b[33m\x1b[1m" << (form == "" ? "" : (form + " "))
                   << "warning:\x1b[0m " << mssg << "\n";
}
void throw_err(string form, string mssg) {
    std::cerr << "\x1b[31m\x1b[1m" << (form == "" ? "" : (form + " "))
            << "error:\x1b[0m " << mssg << "\n";
}
void throw_debug(string form, string mssg) {
    if (VERBOSITY())
        std::cout << "\x1b[34m\x1b[1m" << (form == "" ? "debug" : form)
                  << ":\x1b[0m " << mssg << "\n";
}
void throw_type_err(Expression *exp, std::string type) {
    throw_err("type", "expression '" + exp->toString() + "' does not evaluate as " + type);
}


ApplyExp::ApplyExp(Expression *f, Expression **xs) {
    op = f;
    args = xs;
}
ApplyExp::~ApplyExp() {
    for (int i = 0; args[i]; i++)
        delete args[i];
    delete[] args;
    delete op;
}

IfExp::IfExp(Expression *b, Expression *t, Expression *f) {
    cond = b;
    tExp = t;
    fExp = f;
}

IntExp::IntExp(int n) { val = n; }

// Expression for generating lambdas.
LambdaExp::LambdaExp(string *ids, Expression *rator) {
    xs = ids;
    exp = rator;
}

LetExp::LetExp(string *vs, Expression **xs, Expression *y) {
    ids = vs;
    exps = xs;
    body = y;
}

ListExp::ListExp(Expression** exps) : ListExp::ListExp() {
    for (int i = 0; exps[i]; i++)
        list->add(i, exps[i]);
}

OperatorExp::OperatorExp(Expression *a, Expression *b) {
    left = a;
    right = b;
}

RealExp::RealExp(float n) { val = n; }

SequenceExp::SequenceExp(Expression *a, Expression *b) {
    pre = a;
    post = b;
}

SetExp::SetExp(Expression **xs, Expression **vs) {
    tgts = xs;
    exps = vs;
}


