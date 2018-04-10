#include "expressions/stdlib.hpp"

#include "expression.hpp"
#include "types.hpp"
#include "value.hpp"

#include <cmath>

using namespace std;

LambdaVal* make_fn(string *xs, std::string name, Val (*f)(Env), Type *t, Val (*df)(string, Env, Env) = NULL) {
    auto I = new ImplementExp(f, t, df);
    I->setName(name);
    return new LambdaVal(xs, I);
}

LambdaVal* make_mono_fn(string x, std::string name, Val (*f)(Env), Type *t, Val (*df)(string, Env, Env) = NULL) {
    string *xs = new string[2];
    xs[0] = x;
    xs[1] = "";
    return make_fn(xs, name, f, t, df);
}


// Adds a binding to a library
void add_to_lib(DictVal *lib, string x, Val val) {
    lib->getVals()->add(x, val);
}

Type* type_stdlib(string name) {
    return NULL;
}

Val load_stdlib(string name) {
    return NULL;
}

