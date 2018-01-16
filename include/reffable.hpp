#ifndef _REFFABLE_HPP_
#define _REFFABLE_HPP_

#include "config.hpp"

class Reffable {
    private:
        int refs = 1;

    public:
        virtual ~Reffable() {}
        /**
         * Increments the reference count of the object.
         *
         * Notify the object that another reference has been
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
