#include "expressions/stdlib.hpp"

#include "expression.hpp"
#include "types.hpp"
#include "value.hpp"

#include <cmath>

using namespace std;

LambdaVal* make_fn(string *xs, Val (*f)(Env), Val (*df)(string, Env, Env) = NULL) {
    return new LambdaVal(xs, new ImplementExp(f, df));
}

LambdaVal* make_mono_fn(string x, Val (*f)(Env), Val (*df)(string, Env, Env) = NULL) {
    string *xs = new string[2];
    xs[0] = x;
    xs[1] = "";
    return make_fn(xs, f, df);
}

Val evaluate_sin(Env env) {
    Val v = env->apply("x");
    
    if (!val_is_number(v)) {
        throw_err("type", "call to stdlib function math.sin does not take non-numerical values");
        return NULL;
    }
    
    auto z = typeid(*v) == typeid(IntVal)
            ? ((IntVal*) v)->get()
            : ((RealVal*) v)->get();
    v->rem_ref();

    return new RealVal(sin(z));
}
Val differentiate_sin(string x, Env env, Env denv) {
    Val v = env->apply("x");
    
    if (!val_is_number(v)) {
        throw_err("type", "call to stdlib function math.sin does not take non-numerical values");
        return NULL;
    }

    Val dv = denv->apply("x");

    auto z = typeid(*v) == typeid(IntVal)
            ? ((IntVal*) v)->get()
            : ((RealVal*) v)->get();
    v->rem_ref();


    auto dz = typeid(*dv) == typeid(IntVal)
            ? ((IntVal*) dv)->get()
            : ((RealVal*) dv)->get();
    dv->rem_ref();

    return new RealVal(dz * cos(z));
}


Val evaluate_cos(Env env) {
    Val v = env->apply("x");
    
    if (!val_is_number(v)) {
        throw_err("type", "call to stdlib function math.cos does not take non-numerical values");
        return NULL;
    }
    
    auto z = typeid(*v) == typeid(IntVal)
            ? ((IntVal*) v)->get()
            : ((RealVal*) v)->get();
    v->rem_ref();

    return new RealVal(sin(z));
}
Val differentiate_cos(string x, Env env, Env denv) {
    Val v = env->apply("x");
    
    if (!val_is_number(v)) {
        throw_err("type", "call to stdlib function math.cos does not take non-numerical values");
        return NULL;
    }

    Val dv = denv->apply("x");

    auto z = typeid(*v) == typeid(IntVal)
            ? ((IntVal*) v)->get()
            : ((RealVal*) v)->get();
    v->rem_ref();


    auto dz = typeid(*dv) == typeid(IntVal)
            ? ((IntVal*) dv)->get()
            : ((RealVal*) dv)->get();
    dv->rem_ref();

    return new RealVal(dz * -sin(z));
}

Val evaluate_log(Env env) {
    Val v = env->apply("x");
    
    if (!val_is_number(v)) {
        throw_err("type", "call to stdlib function math.log does not take non-numerical values");
        return NULL;
    }
    
    auto z = typeid(*v) == typeid(IntVal)
            ? ((IntVal*) v)->get()
            : ((RealVal*) v)->get();
    v->rem_ref();
    
    return new RealVal(log(z));
}
Val differentiate_log(string x, Env env, Env denv) {
    Val v = env->apply("x");
    
    if (!val_is_number(v)) {
        throw_err("type", "call to stdlib function math.log does not take non-numerical values");
        return NULL;
    }

    Val dv = denv->apply("x");

    auto z = typeid(*v) == typeid(IntVal)
            ? ((IntVal*) v)->get()
            : ((RealVal*) v)->get();
    v->rem_ref();


    auto dz = typeid(*dv) == typeid(IntVal)
            ? ((IntVal*) dv)->get()
            : ((RealVal*) dv)->get();
    dv->rem_ref();

    return new RealVal(dz / z);
}

Val evaluate_sqrt(Env env) {
    Val v = env->apply("x");
    
    if (!val_is_number(v)) {
        throw_err("type", "call to stdlib function math.sqrt does not take non-numerical values");
        return NULL;
    }
    
    auto z = typeid(*v) == typeid(IntVal)
            ? ((IntVal*) v)->get()
            : ((RealVal*) v)->get();
    v->rem_ref();
    
    return new RealVal(sqrt(z));
}
Val differentiate_sqrt(string x, Env env, Env denv) {
    Val v = env->apply("x");
    
    if (!val_is_number(v)) {
        throw_err("type", "call to stdlib function math.sqrt does not take non-numerical values");
        return NULL;
    }

    Val dv = denv->apply("x");

    auto z = typeid(*v) == typeid(IntVal)
            ? ((IntVal*) v)->get()
            : ((RealVal*) v)->get();
    v->rem_ref();


    auto dz = typeid(*dv) == typeid(IntVal)
            ? ((IntVal*) dv)->get()
            : ((RealVal*) dv)->get();
    dv->rem_ref();

    return new RealVal(dz / (2 * sqrt(z)));
}


// Adds a binding to a library
void add_to_lib(DictVal *lib, string x, Val val) {
    lib->getVals()->add(x, val);
}

DictVal* build_lib_math() {

    DictVal *lib = new DictVal(
        new LinkedList<string>,
        new LinkedList<Val>
    );
    
    add_to_lib(lib, "sin", make_mono_fn("x", evaluate_sin, differentiate_sin));
    add_to_lib(lib, "cos", make_mono_fn("x", evaluate_cos, differentiate_cos));
    add_to_lib(lib, "log", make_mono_fn("x", evaluate_log, differentiate_log));
    add_to_lib(lib, "sqrt", make_mono_fn("x", evaluate_sqrt, differentiate_sqrt));

    return lib;

}

Val load_stdlib(string name) {
    if (name == "math")
        return build_lib_math();
    else
        return NULL;
}

