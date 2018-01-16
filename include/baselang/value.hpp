#ifndef _BASELANG_VALUE_HPP_
#define _BASELANG_VALUE_HPP_

#include "stringable.hpp"
#include "reffable.hpp"

class Value : public Stringable, public Reffable {
    public:
        virtual Value* clone() = 0;
        virtual int set(Value*) = 0;
};
typedef Value* Val;

#endif
