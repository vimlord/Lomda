#ifndef _REFFABLE_HPP_
#define _REFFABLE_HPP_

#include "config.hpp"

/**
 * A custom implementation of a unique pointer scheme that will handle
 * garbage collection as long as it is handled correctly.
 */
class Reffable {
    private:
        // Initially, the object will be unique.
        int refs = 1;

    public:
        virtual ~Reffable() {}

        /**
         * Increments the reference count of the object.
         *
         * Notifies the object that another reference has been
         * created to acknowledge it.
         */
        virtual void add_ref();

        /**
         * Decrements the reference count of the object.
         *
         * Notifies the object that a reference has been lost.
         * If no more references will exist, then the object
         * will be automatically garbage collected.
         */
        virtual void rem_ref();
};

#endif
