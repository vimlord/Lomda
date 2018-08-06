#include "reffable.hpp"

#include "stringable.hpp"

#include <iostream>
using namespace std;

void Reffable::add_ref() {
    ++refs;
}

void Reffable::rem_ref() {
    if (refs == 0) {
        // The item was already deleted; if this point is reached, something
        // is likely wrong unless a special case is reached.
        return;
    } else if (--refs == 0) {
        delete this;
    }
}

