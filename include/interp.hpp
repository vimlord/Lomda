#ifndef _INTERP_HPP_
#define _INTERP_HPP_
#include "value.hpp"

Val run(std::string);

/**
 * Checks the type of a value and unpacks it if it is a thunk.
 * @param v Some value that might be a thunk.
 * @return A value that is not a thunk on success, or NULL on failure.
 */
inline Val unpack_thunk(Val v) {
    // If the function is a thunk, then we need to unpack it.
    while (v && isVal<Thunk>(v)) {
        // Unpack
        throw_debug("thunk", "unpacking thunk defined by " + v->toString());
        Thunk *t = (Thunk*) v;
        v = t->get();
        t->rem_ref();
        throw_debug("thunk", "extracted " + v->toString());
    }

    return v;
}

#endif
