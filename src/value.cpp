#include "value.hpp"
#include "baselang/environment.hpp"

using namespace std;

#include <string>

bool is_zero_val(Val e) {
    if (!e) return false;
    else if (isVal<IntVal>(e))
        return ((IntVal*) e)->get();
    else if (isVal<RealVal>(e))
        return ((RealVal*) e)->get();
    else if (isVal<ListVal>(e)) {
        auto it = ((ListVal*) e)->iterator();

        bool isZero = true;
        while (it->hasNext()) isZero = is_zero_val(it->next());

        return isZero;

    } else return false;
}

// ADTs
AdtVal::~AdtVal() {
    for (int i = 0; args[i]; i++)
        args[i]->rem_ref();
    delete[] args;
}
AdtVal* AdtVal::clone() {
    int i;
    for (i = 0; args[i]; i++);

    Val *xs = new Val[i+1];
    xs[i] = NULL;
    while (i--) {
        xs[i] = args[i];
        xs[i]->add_ref();
    }
    
    return new AdtVal(type, kind, xs);
}
int AdtVal::set(Val v) {
    if (isVal<AdtVal>(v)) {
        // Wipe the array, start anew
        for (int i = 0; args[i]; i++)
            args[i]->rem_ref();
        delete[] args;
        
        // Claim a specific type for the given assignment
        AdtVal *adt = (AdtVal*) v;
        
        // Find the array length
        int i;
        for (i = 0; adt->args[i]; i++);
        
        // Generate the null terminated collection of items
        Val *xs = new Val[i+1];
        xs[i] = NULL;
        while (i--) {
            xs[i] = adt->args[i];
            xs[i]->add_ref();
        }
        
        // Change the names around.
        type = adt->type;
        kind = adt->kind;

        return 0;

    } else
        return 1;
}

// Booleans
BoolVal::BoolVal(bool n) { val = n; }
bool BoolVal::get() { return val; }
int BoolVal::set(Val v) {
    if (isVal<BoolVal>(v)) {
        val = ((BoolVal*) v)->val;
        return 0;
    } else return 1;
}

DictVal::DictVal(std::initializer_list<std::pair<std::string, Val>> elems) {
    // Add each of the elements.
    for (auto pair : elems) {
        add(pair.first, pair.second);
    }
}

DictVal::~DictVal() {
    auto it = iterator();
    while (it->hasNext()) get(it->next())->rem_ref();
    delete it;
}
DictVal* DictVal::clone() {
    auto res = new DictVal;
    
    // Keys
    auto kt = iterator();
    while (kt->hasNext()) {
        string k = kt->next();
        Val v = get(k);

        v->add_ref();
        res->add(k, v);
    }
    delete kt;
    
    // Build the result
    return res;
}
int DictVal::set(Val) { return 1; } // We will not allow setting of fields

// Integers
IntVal::IntVal(int n) { val = n; }
int IntVal::get() { return val; }
int IntVal::set(Val v) {
    if (isVal<IntVal>(v)) {
        val = ((IntVal*) v)->val;
        return 0;
    } else return 1;
}

// Decimals
RealVal::RealVal(float n) { val = n; }
float RealVal::get() { return val; }
int RealVal::set(Val v) {
    if (isVal<RealVal>(v)) {
        val = ((RealVal*) v)->val;
        return 0;
    } else return 1;
}

int TupleVal::set(Val v) {
    if (isVal<TupleVal>(v)) {
        left->rem_ref();
        left = ((TupleVal*) v)->getLeft();
        left->add_ref();

        right->rem_ref();
        right = ((TupleVal*) v)->getRight();
        right->add_ref();

        return 0;
    } else return 1;
}

int StringVal::set(Val v) {
    if (isVal<StringVal>(v)) {
        val = ((StringVal*) v)->get();
        return 0;
    } else return 1;
}

// Lambdas
LambdaVal::LambdaVal(string *ids, Exp exp, Env env) {
    this->xs = ids;
    this->exp = exp;
    this->env = env ? env : new Environment;
}
int LambdaVal::set(Val v) {
    if (isVal<LambdaVal>(v)) {
        LambdaVal *lv = (LambdaVal*) v;
        int i;
    
        // Allocate new memory for the input
        delete[] xs;
        for (i = 0; lv->xs[i] != ""; i++);
        
        // Set a new input set
        xs = new string[i+1];
        xs[i] = "";
        while (i--) xs[i] = lv->xs[i];
        
        // Set a new expression
        delete exp;
        exp = lv->exp->clone();
        
        // Set the environment
        Env e = env;
        env = ((LambdaVal*) v)->env;
        delete e;

        return 0;
    } else return 1;
}
LambdaVal::~LambdaVal() {
    delete[] xs;
    delete exp;
    if (env) env->rem_ref();
}
LambdaVal* LambdaVal::clone() {
    int argc;
    for (argc = 0; xs[argc] != ""; argc++);

    string *ids = new string[argc+1];
    ids[argc] = "";
    while (argc--) ids[argc] = xs[argc];
    
    env->add_ref();
    return new LambdaVal(ids, exp->clone(), env);
}
Val LambdaVal::apply(Val *argv, Env e) {
    Env E = e ? e : env;
    
    E = new Environment(E);

    // Verify that the correct number of arguments have been provided
    for (int i = 0; argv[i] || xs[i] != ""; i++)
        if ((!argv[i]) != (xs[i] == "")) {
            // If one of the lists ends before the other, the input is bad.
            delete E;
            return NULL;
        }
    
    // Create an environment for handling the processing
    for (int i = 0; argv[i]; i++)
        E->set(xs[i], argv[i]);

    // Compute the result
    Val res = exp->evaluate(E);
    
    // Garbage collection
    E->rem_ref();

    // Return it
    return res;
}
void LambdaVal::setEnv(Env e) {
    Env tmp = env;
    env = e;
    
    if (env) env->add_ref();
    if (tmp) tmp->rem_ref();
}

ListVal::~ListVal() {
    for (int i = 0; i < size(); i++)
        get(i)->rem_ref();
}
ListVal* ListVal::clone() {
    // Add a copy of each element of the list
    ListVal *res = new ListVal;
    
    for (int i = 0; i < size(); i++) {
        Val v = get(i);
        res->add(i, v);
        if (v) v->add_ref();
    }

    // Give it back
    return res;
}
int ListVal::set(Val v) {
    if (isVal<ListVal>(v)) {
        // Clear the list
        while (!isEmpty())
            remove(size()-1)->rem_ref();
        
        // Add each value to the new list
        auto vs = (ListVal*) v;
        while (!vs->isEmpty()) {
            auto v = vs->remove(0);
            add(size(), v);
        }
        
        return 0;
    } else return 1;
}

int Thunk::set(Val v) {
    if (isVal<Thunk>(v)) {
        Thunk *t = (Thunk*) v;
        
        if (val) val->rem_ref();
        delete exp;

        if (t->val) t->val->add_ref();
        
        exp = t->exp->clone();
        val = t->val;

        return 0;
    } else return 1;
}
Val Thunk::get(Env e) {
    if (!e) e = env;
    if (!val) {
        // Preemptive debug print
        throw_debug("thunk", "evaluating " + exp->toString() + " | " + e->toString());

        val = exp->evaluate(e);

        if (val && configuration.verbosity) std::cout << "\x1b[34m\x1b[1mthunk:\x1b[0m created new ref " << val << "\n";
    }

    // Debug print the outcome
    if (val) {
        val->add_ref();
        throw_debug("thunk ", "" + exp->toString() + " | " + e->toString() + " ↓ " + val->toString());
        return val;
    } else {
        throw_debug("thunk", "" + exp->toString() + " | " + e->toString() + " cannot be computed");
        return NULL;
    }
}

