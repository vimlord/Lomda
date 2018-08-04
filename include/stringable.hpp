#ifndef _STRINGABLE_HPP_
#define _STRINGABLE_HPP_

#include <string>

class Stringable {
    public:
        virtual ~Stringable() {}

        /**
         * Constructs a string representation of the object.
         *
         * @return A string representing the object.
         */
        virtual std::string toString() = 0;
};

/**
 * Displays a Stringable object.
 *
 * @param os The output stream.
 * @param obj The Stringable object to display.
 *
 * @return The output stream.
 */
std::ostream &operator<<(std::ostream &os, Stringable &exp);

#endif
