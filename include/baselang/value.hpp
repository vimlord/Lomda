#ifndef _BASELANG_VALUE_HPP_
#define _BASELANG_VALUE_HPP_

#include "stringable.hpp"

class Value : public Stringable {
    private:
        int refs = 1;
    public:
        virtual ~Value() {}

        virtual Value* clone() = 0;
        virtual int set(Value*) = 0;
        
        /**
         * Increments the reference count of the object.
         *
         * Notify the object that another reference has been
         * created to acknowledge it.
         */
         void add_ref();

        /**
         * Decrements the reference count of the object.
         *
         * Notifies the object that a reference has been lost.
         * If no more references will exist, then the object
         * will be automatically garbage collected.
         */
        void rem_ref();
};

#endif
