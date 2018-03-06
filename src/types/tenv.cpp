#include "types.hpp"
#include "proof.hpp"

using namespace std;

TypeEnv::~TypeEnv() {
    for (auto it : types)
        delete it.second;

    for (auto it : mgu)
        delete it.second;
}

Type* TypeEnv::apply(string x) {
    if (types.find(x) == types.end()) {
        // We need to instantiate this type to be a new variable type
        auto V = make_tvar();
        types[x] = V;
    }
    return types[x]->clone();
}

bool TypeEnv::hasVar(string x) {
    return types.find(x) != types.end();
}

int TypeEnv::set(string x, Type* v) {
    int res = 0;
    if (types.find(x) != types.end()) {
        delete types[x];
        res = 1;
    }
    types[x] = v;
    return res;
}

Tenv TypeEnv::clone() {
    Tenv env = new TypeEnv;
    for (auto it : types)
        env->set(it.first, it.second->clone());
    for (auto it : mgu)
        env->set_tvar(it.first, it.second->clone());

    env->next_id = next_id;

    return env;
}

Type* TypeEnv::get_tvar(string v) {
    if (mgu.find(v) != mgu.end())
        return mgu[v];
    else
        return NULL;
}

void TypeEnv::set_tvar(string v, Type *t) {
    rem_tvar(v); mgu[v] = t;
}

void TypeEnv::rem_tvar(string v) {
    if (mgu.find(v) != mgu.end()) {
        delete mgu[v];
        mgu.erase(v);
    }
}

Type* TypeEnv::make_tvar() {
    auto V = new VarType(next_id);
    show_proof_step("Let " + next_id + " be a fresh type variable");

    // Increment the next_id var
    int i;
    for (i = next_id.length()-1; i >= 0 && next_id[i] == 'z'; i--)
        next_id[i] = 'a';
    if (i < 0)
        next_id = "a" + next_id;
    else
        next_id[i]++;

    mgu[V->toString()] = V;

    return V->clone();
}


