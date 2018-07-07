#ifndef _PARSER_HPP_
#define _PARSER_HPP_

#include "expression.hpp"

#include <string>

/**
 * Given a string containing a program, generates an AST that can be evaluated
 * to yield a value. Will print an error if parsing fails.
 * @param program A string that represents a program.
 * @return An AST on success, or NULL on failure.
 */
Exp parse_program(std::string program);

#endif
