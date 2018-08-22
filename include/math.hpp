#ifndef _MATH_HPP_
#define _MATH_HPP_
#include "value.hpp"

/**
 * Computes a deep dot product between two tensors.
 */
Val dot(Val, Val);

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

int is_square_matrix(Val v);

/**
 * Determines whether or not a given value is a vector.
 * @param v The value to check.
 * @return The length the vector or -1 if not a vector.
 */
int is_vector(Val v);

/**
 * Computes the sum of two values.
 * @param a The left hand value.
 * @param b The right hand value.
 * @return a + b
 */
Val add(Val a, Val b);

/**
 * Computes the difference between two values.
 * @param a The left hand value.
 * @param b The right hand value.
 * @return a - b
 */
Val sub(Val a, Val b);

/**
 * Computes the product of two values.
 * @param a The left hand value.
 * @param b The right hand value.
 * @return a * b
 */
Val mult(Val a, Val b);

/**
 * Computes the quotient of two values.
 * @param a The left hand value.
 * @param b The right hand value.
 * @return a / b
 */
Val div(Val a, Val b);

/**
 * Computes the inverse of an expression.
 * @param x A value to compute an inverse of.
 * @return x^-1 = 1/x
 */
Val inv(Val x);

/**
 * Generates an identity matrix of a given size.
 * @param n The dimensions of the matrix.
 * @return An n by n identity matrix.
 */
Val identity_matrix(int n);

#endif
