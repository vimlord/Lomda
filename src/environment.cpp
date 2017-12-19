#include "baselang/value.hpp"
#include "environment.hpp"
#include "baselang/expression.hpp"
#include "baselang/value.hpp"

#include "structures/list.hpp"

#include <iostream>

using namespace std;


EmptyEnv::EmptyEnv() {}
ExtendEnv::ExtendEnv(string id, Value* val, Environment* env) {
    // Store the id to track
    this->id = id;

    // The subenvironment
    subenv = env;
    
    // Setup the reference
    ref = val;
    if (ref) ref->add_ref();
}

EmptyEnv::~EmptyEnv() {
    //std::cout << "deleting env " << this << " (" << *this << ")\n";
}
ExtendEnv::~ExtendEnv() {
    //std::cout << "deleting env " << this << " (" << *this << ")\n";
    if (ref) {
        Value *v = ref;
        ref = NULL;
        v->rem_ref();
    }
    delete subenv;
}

Value* EmptyEnv::apply(string id) {
    return NULL;
}
Value* ExtendEnv::apply(string id) {
    // Attempt to lookup the item
    if (!id.compare(this->id))
        return ref;
    else if (subenv)
        return subenv->apply(id);
    else
        return NULL;
}

Environment* EmptyEnv::clone() { return new EmptyEnv(); }
Environment* ExtendEnv::clone() {
    return new ExtendEnv(id, ref, subenv->clone());
}

Environment* Environment::subenvironment() { 
    return this->subenv;
}


