#ifndef _STDLIB_HPP_
#define _STDLIB_HPP_

#include "types.hpp"
#include "baselang/value.hpp"

// Loads a standard library with a given name, if possible
Type* type_stdlib(std::string);
Val load_stdlib(std::string);

// File system libearies
Type* type_stdlib_fs();
Val load_stdlib_fs();

// Linear algebra libraries
Type* type_stdlib_linalg();
Val load_stdlib_linalg();

// Math libraries
Type* type_stdlib_math();
Val load_stdlib_math();

// Defines the string library
Type* type_stdlib_string();
Val load_stdlib_string();

// Sorting libraries
Type* type_stdlib_sort();
Val load_stdlib_sort();

// System libs
Type* type_stdlib_sys();
Val load_stdlib_sys();

// Random numbers
Type* type_stdlib_random();
Val load_stdlib_random();

#endif
