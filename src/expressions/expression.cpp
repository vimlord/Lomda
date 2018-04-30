#include "expression.hpp"

#include "value.hpp"
#include "config.hpp"

#include <string>

using namespace std;

Exp reexpress(Val v) {
    if (!v) return NULL;
    else if (typeid(*v) == typeid(IntVal))
        return new IntExp(((IntVal*) v)->get());
    else if (typeid(*v) == typeid(RealVal))
        return new RealExp(((RealVal*) v)->get());
    else if (typeid(*v) == typeid(BoolVal))
        return ((BoolVal*) v)->get() 
                ? (Exp) new TrueExp
                : (Exp) new FalseExp;
    else if (typeid(*v) == typeid(ListVal)) {
        ListVal *lv = (ListVal*) v;
        LinkedList<Exp> *list = new LinkedList<Exp>;

        auto it = lv->get()->iterator();
        while (it->hasNext()) list->add(list->size(), reexpress(it->next()));

        return new ListExp(list);
    } else if (typeid(*v) == typeid(LambdaVal)) {
        LambdaVal *lv = (LambdaVal*) v;

        int argc = 0;
        while (lv->getArgs()[argc] != "");

        std::string *xs = new std::string[argc+1];
        xs[argc] = "";
        while (argc--) xs[argc] = lv->getArgs()[argc];

        return new LambdaExp(xs, lv->getBody());
    } else
        return NULL;
}

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
void throw_type_err(Exp exp, std::string type) {
    throw_err("type", "expression '" + exp->toString() + "' does not evaluate as " + type);
}


ApplyExp::ApplyExp(Exp f, Exp *xs) {
    op = f;
    args = xs;
}
ApplyExp::~ApplyExp() {
    for (int i = 0; args[i]; i++)
        delete args[i];
    delete[] args;
    delete op;
}

DictExp::DictExp(std::initializer_list<std::pair<std::string, Exp>> es) : DictExp() {
    for (auto it : es) {
        keys->add(0, it.first);
        vals->add(0, it.second);
    }
}

IfExp::IfExp(Exp b, Exp t, Exp f) {
    cond = b;
    tExp = t;
    fExp = f;
}

IntExp::IntExp(int n) { val = n; }

// Expression for generating lambdas.
LambdaExp::LambdaExp(string *ids, Exp rator) {
    xs = ids;
    exp = rator;
}

LetExp::LetExp(string *vs, Exp *xs, Exp y, bool *r) {
    ids = vs;
    exps = xs;
    body = y;
    rec = r;
}

ListExp::ListExp(Expression** exps) : ListExp::ListExp() {
    for (int i = 0; exps[i]; i++)
        list->add(i, exps[i]);
}

OperatorExp::OperatorExp(Exp a, Exp b) {
    left = a;
    right = b;
}

RealExp::RealExp(float n) { val = n; }

Exp SequenceExp::clone() {
    auto es = new LinkedList<Exp>;

    auto it = seq->iterator();
    while (it->hasNext())
        es->add(es->size(), it->next()->clone());
    delete it;

    return new SequenceExp(es);

}

SetExp::SetExp(Exp xs, Exp vs) {
    tgt = xs;
    exp = vs;
}


