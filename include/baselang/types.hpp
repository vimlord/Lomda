#ifndef _BASELANG_TYPES_HPP_
#define _BASELANG_TYPES_HPP_

#include "stringable.hpp"
#include "reffable.hpp"

class Type : public Stringable, public Reffable {
    public:
        virtual Type* clone() = 0;
};

#endif
