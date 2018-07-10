#include "baselang/value.hpp"
#include "baselang/environment.hpp"
#include "baselang/expression.hpp"
#include "baselang/value.hpp"

#include "structures/list.hpp"

#include <iostream>

using namespace std;

Environment::Environment(Env env) {
    subenv = env;
    if (env) env->add_ref();
}
Environment::~Environment() {
    //std::cout << "deleting env " << this << " (" << *this << ")\n";
    for (auto it : store) {
        Val v = it.second;
        store[it.first] = NULL;
        v->rem_ref();
    }
}

Val Environment::apply(string x) {
    if (store.find(x) != store.end())
        return store[x];
    else
        return subenv ? subenv->apply(x) : NULL;
}

int Environment::set(string x, Val v) {
    if (!v) return 1;

    // Remove the reference if necessary
    if (store.find(x) != store.end())
        store[x]->rem_ref();
    // Set the new value in the store
    store[x] = v;
    v->add_ref();

    return 0;
}

void Environment::rem(string x) {
    if (apply(x)) {
        // If the slot is filled, we clear it
        store[x]->rem_ref();
        store.erase(x);
    }
}

/*
~ExtendEnv() {
    //std::cout << "deleting env " << this << " (" << *this << ")\n";
    if (ref) {
        Value *v = ref;
        ref = NULL;
        v->rem_ref();
    }
}*/

Env Environment::clone() {
    Env env = new Environment(subenv ? subenv->clone() : NULL);
    for (auto it : store) {
        env->store[it.first] = it.second;
    }

    return env;
}


