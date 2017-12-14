#ifndef _BASELANG_VALUE_HPP_
#define _BASELANG_VALUE_HPP_

#include "stringable.hpp"

class Value : public Stringable {
    public:
        virtual Value* clone() = 0;
        virtual int set(Value*) = 0;
};

#endif
