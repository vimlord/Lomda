#ifndef _STDLIB_HPP_
#define _STDLIB_HPP_

#include "types.hpp"
#include "baselang/value.hpp"

// Loads a standard library with a given name, if possible
Type* type_stdlib(std::string);
Val load_stdlib(std::string);

// Defines the string library
Type* type_stdlib_string();
Val load_stdlib_string();

// Sorting libraries
Type* type_stdlib_sort();
Val load_stdlib_sort();

#endif
