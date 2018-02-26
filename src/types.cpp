#include "types.hpp"
#include "expression.hpp"

using namespace std;

string VarType::NEXT_ID = "a";
int TypeEnv::set(string x, Type* v) {
    v->add_ref();
    int res = 0;
    if (types.find(x) != types.end()) {
        types[x]->rem_ref();
        res = 1;
    }
    types[x] = v;
    return res;
}

Tenv TypeEnv::clone() {
    Tenv env = new TypeEnv;
    for (auto it : types) {
        env->set(it.first, it.second);
        it.second->add_ref();
    }
    return env;
}

string TypeEnv::toString() {
    string res = "{";
    int i = 0;
    for (auto it : types) {
        if (i) res += ", ";
        res += it.first + " := " + it.second->toString();
        i++;
    }
}

Type* LetExp::typeOf(Tenv tenv) {
    unordered_map<string, Type*> tmp;
    
    int i;
    for (i = 0; exps[i]; i++) {
        auto t = exps[i]->typeOf(tenv);
        if (!t) {
            break;
        } else {
            auto s = tenv->apply(ids[i]);
            if (s) {
                // We may have to suppress variables.
                s->add_ref();
                tmp[ids[i]] = s;
            }
            tenv->set(ids[i], t);
        }
    }

    // Evaluate the body if possible
    auto T = exps[i] ? NULL : body->typeOf(tenv);
    
    // Restore the tenv
    for (auto it : tmp) {
        tenv->set(it.first, it.second);
        tmp.erase(it.first);
    }

    return T;

}


Type* VarExp::typeOf(Tenv tenv) {
    auto T = tenv->apply(id);
    if (!T) {
        // We will create a variable to represent the variable
        tenv->set(id, T = new VarType);
    } else
        T->add_ref();
    return T;
}


