#ifndef _BASELANG_VALUE_HPP_
#define _BASELANG_VALUE_HPP_

#include "stringable.hpp"
#include "reffable.hpp"

class Value : public Stringable, public Reffable {
    public:
        virtual ~Value() {}

        virtual Value* clone() = 0;
        virtual int set(Value*) { return 1; };
};
typedef Value* Val;

#endif
