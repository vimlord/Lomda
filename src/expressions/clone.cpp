#include "expression.hpp"

using namespace std;

Exp ApplyExp::clone() {
    int i;
    for (i = 0; args[i]; i++);

    Exp *a = new Exp[i+1];
    a[i] = NULL;

    while (i--) {
        a[i] = args[i]->clone();
    }

    return new ApplyExp(op->clone(), a);
}

Exp LambdaExp::clone() {
    int i;
    for (i = 0; xs[i] != ""; i++);
    
    string *ids = new string[i+1];
    ids[i] = "";
    while (i--)
        ids[i] = xs[i];

    return new LambdaExp(ids, exp->clone());

}

Exp LetExp::clone() {
    int argc;
    for (argc = 0; exps[argc]; argc++);
    
    string *ps = new string[argc+1];
    Exp *es = new Exp[argc+1];
    ps[argc] = "";
    es[argc] = NULL;
    while (argc--) {
        ps[argc] = ids[argc];
        es[argc] = exps[argc]->clone();
    }

    return new LetExp(ps, es, body->clone());

}

Exp ListExp::clone() {
    LinkedList<Exp> *exps = new LinkedList<Exp>;

    auto it = list->iterator();
    while (it->hasNext())
        exps->add(exps->size(), it->next()->clone());
    delete it;

    return new ListExp(exps);

}

Exp PrintExp::clone() {
    int i;
    for (i = 0; args[i]; i++);

    Exp *list = new Exp[i+1];
    list[i] = NULL;
    while (i--) list[i] = args[i]->clone();

    return new PrintExp(list);
}

Exp SetExp::clone() {
    int i;
    for (i = 0; exps[i]; i++);

    Exp *a = new Exp[i+1];
    Exp *b = new Exp[i+1];
    a[i] = b[i] = NULL;

    while (i--) {
        a[i] = tgts[i];
        b[i] = exps[i];
    }

    return new SetExp(a, b);
}

