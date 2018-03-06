#ifndef _PROOF_HPP_
#define _PROOF_HPP_

#include "baselang/types.hpp"
#include "baselang/expression.hpp"

void show_proof_step(std::string x);
void show_proof_therefore(std::string x);
void show_mgu_step(Tenv t, Type *a, Type *b, Type *c);
std::string type_res_str(Tenv tenv, Exp exp, Type *type);

#endif
