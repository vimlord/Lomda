#ifndef _MATH_HPP_
#define _MATH_HPP_
#include "value.hpp"

/**
 * Computes the natural log of an expression, as ln(x) would.
 * For matrices, this is done using a Taylor approximation combined
 * with a scaling of the matrix to permit the usage of this method.
 * @param v A value to compute logarithm of.
 * @return ln(v) on success otherwise NULL.
 */
Val log(Val);

/**
 * Comoutes the result of raising e to the power of a given value.
 * For matrices, this is done using Taylor approximations.
 * @param v A value to exponentiate.
 * @return e^v if exponentiable otherwise NULL.
 */
Val exp(Val);

/**
 * Computes the result of raising a value to the power of another.
 * Integer powers are simplified using power rules, while real
 * numbers will resort to the C++ backend. Matrices are computed
 * using log and exp rules.
 */
Val pow(Val, Val);

/**
 * Determines whether or not a given value is a matrix.
 * @param v The value to check
 * @return The dimensions if a matrix, NULL otherwise.
 */
int* is_matrix(Val v);

/**
 * Determines whether or not a given value is a vector.
 * @param v The value to check.
 * @return The length the vector or -1 if not a vector.
 */
int is_vector(Val v);

Val add(Val, Val);
Val sub(Val, Val);
Val mult(Val, Val);
Val div(Val, Val);
Val inv(Val);

#endif
