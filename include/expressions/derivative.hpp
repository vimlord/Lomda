#ifndef _EXPRESSIONS_DERIVATIVE_HPP_
#define _EXPRESSIONS_DERIVATIVE_HPP_

#include "baselang/expression.hpp"

Val deriveConstVal(std::string, Val, int = 1);
Val deriveConstVal(std::string, Val, Val, int = 1);

#endif
