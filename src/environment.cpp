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
}

void EmptyEnv::destroy(int) {}
void ExtendEnv::destroy(int depth) {
    // The variable will no longer be in scope. So, we will delete
    // the value from memory to save some space.
    delete ref;
    
    // Keep destroying
    if (depth > 0)
        subenv->destroy(depth-1);
    else if (depth < 0)
        subenv->destroy(depth);
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
    return new ExtendEnv(*this);
}

Environment* Environment::subenvironment() { 
    return this->subenv;
}




