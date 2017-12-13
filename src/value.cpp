#include "value.hpp"
#include "environment.hpp"

using namespace std;

#include <string>

// Booleans
BoolVal::BoolVal(bool n) { val = n; }
bool BoolVal::get() { return val; }
int BoolVal::set(Value *v) {
    if (typeid(*v) == typeid(BoolVal)) {
        val = ((BoolVal*) v)->val;
        return 0;
    } else return 1;
}

// Integers
IntVal::IntVal(int n) { val = n; }
int IntVal::get() { return val; }
int IntVal::set(Value *v) {
    if (typeid(*v) == typeid(IntVal)) {
        val = ((IntVal*) v)->val;
        return 0;
    } else return 1;
}

// Decimals
RealVal::RealVal(float n) { val = n; }
float RealVal::get() { return val; }
int RealVal::set(Value *v) {
    if (typeid(*v) == typeid(RealVal)) {
        val = ((RealVal*) v)->val;
        return 0;
    } else return 1;
}

// Lambdas
LambdaVal::LambdaVal(string *ids, Expression *exp, Environment *env) {
    this->xs = ids;
    this->exp = exp;
    this->env = env;
}
int LambdaVal::set(Value *v) {
    if (typeid(*v) == typeid(LambdaVal)) {
        xs = ((LambdaVal*) v)->xs;
        exp = ((LambdaVal*) v)->exp;
        env = ((LambdaVal*) v)->env;
        return 0;
    } else return 1;
}

Value* LambdaVal::apply(Value **argv, Environment *e) {
    Environment *E = e ? e : env;

    // Verify that the correct number of arguments have been provided
    for (int i = 0; argv[i] || xs[i] != ""; i++)
        if ((!argv[i]) != (xs[i] == ""))
            // If one of the lists ends before the other, the input is bad.
            return NULL;
    
    // Create an environment for handling the processing
    for (int i = 0; argv[i]; i++)
        E = new ExtendEnv(xs[i], argv[i], E);

    // Compute the result
    Value *res = exp->valueOf(E);

    // Return it
    return res;
}
void LambdaVal::setEnv(Environment *e) {
    env = e;
}

ListVal* ListVal::copy() {
    // Add a copy of each element of the list
    //Iterator<int, Value*> *it = list->iterator();
    ListVal *res = new ListVal(list);
    /*for (int i = 0; it->hasNext(); i++)
        res->list->add(i, it->next()->copy());*/

    // Give it back
    return res;
}
int ListVal::set(Value *v) {
    if (typeid(*v) == typeid(ListVal)) {
        list = ((ListVal*) v)->list;
        return 0;
    } else return 1;
}



