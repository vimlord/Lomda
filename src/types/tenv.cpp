#include "types.hpp"
#include "proof.hpp"

using namespace std;

string TypeEnv::next_id = "a";

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
    else {
        mgu[v] = new VarType(v);
        return mgu[v];
    }
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
    // Increment the next_id var
    int i;
    for (i = next_id.length()-1; i >= 0 && next_id[i] == 'z'; i--)
        next_id[i] = 'a';
    if (i < 0)
        next_id = "a" + next_id;
    else
        next_id[i]++;

    auto V = new VarType(next_id);
    show_proof_step("Let " + next_id + " be a fresh type variable.");

    mgu[next_id] = V;

    return V->clone();
}


Tenv TypeEnv::unify(Tenv other, Tenv scope) {
    Tenv tenv = new TypeEnv;;

    // Merge the MGUs
    for (auto it : mgu) {
        string s = it.first;
        if (other->mgu.find(s) != other->mgu.end()) {
            auto T = it.second->unify(other->mgu[s], scope);
            if (!T) {
                delete tenv;
                return NULL;
            } else
                tenv->types[s] = T;
        } else {
            tenv->types[s] = it.second->clone();
        }
    }
    for (auto it : other->mgu) {
        string s = it.first;
        if (tenv->mgu.find(s) != tenv->mgu.end()) {
            tenv->types[s] = it.second->clone();
        }
    }
    
    // Merge the types
    for (auto it : types) {
        string s = it.first;
        if (other->hasVar(s)) {
            auto T = it.second->unify(other->types[s], scope);
            if (!T) {
                delete tenv;
                return NULL;
            } else
                tenv->types[s] = T;
        } else {
            tenv->types[s] = it.second->clone();
        }
    }
    for (auto it : other->types) {
        string s = it.first;
        if (!tenv->hasVar(s)) {
            tenv->types[s] = it.second->clone();
        }
    }

    return tenv;
}


