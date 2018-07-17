#include "expression.hpp"

#include "value.hpp"
#include "config.hpp"

#include <string>

using namespace std;


void throw_warning(string form, string mssg) {
    if (configuration.werror) throw_err(form, mssg);
    else std::cerr << "\x1b[33m\x1b[1m" << (form == "" ? "" : (form + " "))
                   << "warning:\x1b[0m " << mssg << "\n";
}
void throw_err(string form, string mssg) {
    std::cerr << "\x1b[31m\x1b[1m" << (form == "" ? "" : (form + " "))
            << "error:\x1b[0m " << mssg << "\n";
}
void throw_debug(string form, string mssg) {
    if (configuration.verbosity)
        std::cout << "\x1b[34m\x1b[1m" << (form == "" ? "debug" : form)
                  << ":\x1b[0m " << mssg << "\n";
}
void throw_type_err(Exp exp, std::string type) {
    throw_err("type", "expression '" + exp->toString() + "' does not evaluate as " + type);
}

Exp AdtExp::clone() {
    int i;
    for (i = 0; args[i]; i++);

    Exp *xs = new Exp[i+1];
    xs[i] = NULL;
    while (i--) xs[i] = args[i]->clone();

    return new AdtExp(name, kind, xs);

}

AdtDeclarationExp::~AdtDeclarationExp() {
    for (int i = 0; argss[i]; i++) {
        for (int j = 0; argss[i][j]; j++)
            delete argss[i][j];
        delete[] argss[i];
    }
    delete[] argss;
    delete[] ids;

    delete body;
}
Exp AdtDeclarationExp::clone() {
    int i;
    for (i = 0; argss[i]; i++);
    
    string *xs = new string[i+1];
    Type* **ass = new Type**[i+1];
    xs[i] = ""; ass[i] = NULL;

    while (i--) {
        xs[i] = ids[i];

        int j; for (j = 0; argss[i][j]; j++);
        ass[i] = new Type*[j+1];
        ass[i][j] = NULL;
        while (j--) ass[i][j] = argss[i][j]->clone();
    }
    
    return new AdtDeclarationExp(name, xs, ass, body->clone());
}


SwitchExp::~SwitchExp() {
    for (int i = 0; bodies[i]; i++) {
        delete bodies[i];
        delete[] idss[i];
    }
    
    delete adt;
    delete[] bodies;
    delete[] idss;
    delete[] names;
}
Exp SwitchExp::clone() {
    int i; for (i = 0; bodies[i]; i++);

    auto bs = new Exp[i+1];
    auto ns = new string[i+1];
    auto iss = new string*[i+1];
    bs[i] = NULL;
    ns[i] = "";
    iss[i] = NULL;
    
    while (i--) {
        bs[i] = bodies[i]->clone();
        ns[i] = names[i];

        int j; for (j = 0; idss[i][j] != ""; j++);
        iss[i] = new string[j+1];
        iss[i][j] = "";
        while (j--) iss[i][j] = idss[i][j];
    }
    
    return new SwitchExp(adt->clone(), ns, iss, bs);
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

ListExp::ListExp() {}

ListExp::ListExp(Exp* exps) : ListExp::ListExp() {
    for (int i = 0; exps[i]; i++)
        add(i, exps[i]);
}

ListExp::ListExp(List<Exp> *list) {
    while (list->size())
        add(size(), list->remove(0));
    delete list;
}

ListExp::~ListExp() {
    while (!isEmpty())
        delete remove(size()-1);
}

std::map<std::string, Val> ImportExp::module_cache;
void ImportExp::clear_cache() {
    // Delete the stored module.
    for (auto it : module_cache)
        it.second->rem_ref();
    
    // Wipe the cache clean.
    module_cache.clear();
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


