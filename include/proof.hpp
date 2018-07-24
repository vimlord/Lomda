#ifndef _PROOF_HPP_
#define _PROOF_HPP_

#include "baselang/types.hpp"
#include "baselang/expression.hpp"

/**
 * Displays information about a step in a proof.
 * @param x The statement to make.
 */
void show_proof_step(std::string x);

/**
 * Displays an expression of the form "Therefore, x"
 * @param x The statement that is stated to be true.
 */
void show_proof_therefore(std::string x);

/**
 * Shows a step in the reduction of an MGU.
 * @param t The environment under which the check occcurs.
 * @param a The left-hand type to check.
 * @param b The right-hand type to check.
 * @param c The specific reduction resulting from saying that a = b.
 */
void show_mgu_step(Tenv t, Type *a, Type *b, Type *c);

/**
 * Generates a string denoting a typing result.
 * @param tenv A type environment.
 * @param exp The expression that was typed.
 * @param type The type of the expression given the type environment.
 * @return tenv |- exp : type
 */
std::string type_res_str(Tenv tenv, Exp exp, Type *type);

#endif
