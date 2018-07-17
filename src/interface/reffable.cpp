#include "reffable.hpp"

#include "stringable.hpp"

#include <iostream>
using namespace std;

void Reffable::add_ref() {
    ++refs;
    
    if (configuration.verbosity) {
        std::cout << "\x1b[34m\x1b[1mmem_mgt:\x1b[0m "
                  << "ref count of " << this
                  << " up to "
                  << refs << "\n";
    }
}

void Reffable::rem_ref() {
    if (refs == 0) {
        std::cout << "\x1b[34m\x1b[1mmem_mgt:\x1b[0m "
                  << "ref " << this << " is already zero\n";
        return;
    }

    --refs;
    
    if (configuration.verbosity) {
        std::cout << "\x1b[34m\x1b[1mmem_mgt:\x1b[0m "
                  << "ref count of " << this
                  << " down to "
                  << refs << "\n";
    }

    if (refs == 0) {
        delete this;
    }
}

