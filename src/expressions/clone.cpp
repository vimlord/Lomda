#include "expression.hpp"

using namespace std;

Expression* ApplyExp::clone() {
    int i;
    for (i = 0; args[i]; i++);

    Expression **a = new Expression*[i+1];
    a[i] = 0;

    while (i--) {
        a[i] = args[i];
    }

    return new ApplyExp(op->clone(), a);
}

Expression* LambdaExp::clone() {
    int i;
    for (i = 0; xs[i] != ""; i++);
    
    string *ids = new string[i+1];
    ids[i] = "";
    while (i--)
        ids[i] = xs[i];

    return new LambdaExp(ids, exp->clone());

}

Expression* LetExp::clone() {
    int argc;
    for (int argc = 0; exps[argc]; argc++);
    
    string *ps = new string[argc+1];
    Expression **es = new Expression*[argc+1];
    ps[argc] = "";
    es[argc] = NULL;
    while (argc--) {
        ps[argc] = ids[argc];
        es[argc] = exps[argc]->clone();
    }

    return new LetExp(ps, es, body->clone());

}

Expression* ListExp::clone() {
    LinkedList<Expression*> *exps = new LinkedList<Expression*>;

    auto it = list->iterator();
    while (it->hasNext())
        exps->add(exps->size(), it->next()->clone());
    delete it;

    return new ListExp(exps);

}

Expression* SetExp::clone() {
    int i;
    for (i = 0; exps[i]; i++);

    Expression **a = new Expression*[i+1];
    Expression **b = new Expression*[i+1];
    a[i] = b[i] = NULL;

    while (i--) {
        a[i] = tgts[i];
        b[i] = exps[i];
    }

    return new SetExp(a, b);
}

