#ifndef _STRINGABLE_HPP_
#define _STRINGABLE_HPP_

#include <string>

class Stringable {
    public:
        virtual std::string toString() = 0;
};

std::ostream &operator<<(std::ostream &os, Stringable &exp);

#endif
