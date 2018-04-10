#ifndef _MATH_HPP_
#define _MATH_HPP_
#include "value.hpp"

Val log(Val);
Val exp(Val);
Val pow(Val, Val);

int* is_matrix(Val v);
int is_vector(Val v);

#endif
