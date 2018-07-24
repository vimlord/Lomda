#ifndef _PARSER_HPP_
#define _PARSER_HPP_

#include "expression.hpp"

#include <string>
#include <list>

/**
 * Given a string containing a program, generates an AST that can be evaluated
 * to yield a value. Will print an error if parsing fails.
 * @param program A string that represents a program.
 * @return An AST on success, or NULL on failure.
 */
Exp parse_program(std::string program);

// We define a special type for storing result<Expression> lengths.
template<typename T>
struct result {
    T* value = NULL;
    int strlen = -1;
    void reset() {
        if (value) {
            delete value;
            value = NULL;
        }
        strlen = -1;
    }
};

/**
 * Extracts an expression complying with the order of operations defined
 * by PEMDAS.
 * @param str The program to parse.
 * @param order The order in the PEMDAS hierarchy to use; used internally.
 */
result<Expression> parse_pemdas(std::string str, int order = 13);

/**
 * Parses a statement denoting a single line of a program.
 * @param str The program to parse.
 * @param ends Whether or not the entire string must be consumed.
 * @return An expression and its length in the string, or an indicator
 *          of failure if an expression could not be extracted.
 */
result<Expression> parse_body(std::string str, bool ends = false);

/**
 * Given a pairing of opening/closing characters, find the index of closure.
 *
 * @param str A string of the form aSbS, S not necessarily equivalent.
 * @param a A recognized opening character.
 * @param b A recognized closing character.
 * @return The index of the closing b, or -1 if not closed.
 */
int index_of_closure(std::string str, char a, char b);
int index_of_char(std::string str, char c);

/**
 * Given a string, returns the amount of characters to trim to traverse
 * over the literal and clear whitespace, or -1 if not present.
 */
int starts_with(std::string str, std::string lit);
int starts_with_any(std::string str, std::list<std::string> lit);

/**
 * We know that a program is a series of expressions split by semicolons.
 * Thus, we seek to perform tokenization split by semicolons.
 */
std::list<std::string> extract_statements(std::string, char = ';', bool = true);

/**
 * Parses a type expression from a given string.
 */
result<Type> parse_type(std::string str);

/**
 * Using order of operations, parse the string for an expression.
 * @param str  The string to extract a PEMDAS operation from
 * @param order The location in the hierarchy to search at
 */
result<Expression> parse_pemdas(std::string str, int order);

/**
 * Given a string containing a single "command", parse the
 * command and generate an appropriate expression.
 * @param program A string containing a (hopefully) valid program.
 * @return A pointer to an AST on success or NULL on failure.
 */
Exp parse_statement(std::string program);

/**
 * Processes a sequence of statements and creates a program.
 * @param program The first line of a program.
 * @param tail The rest of the program.
 * @return An expression on success or NULL on failure.
 */
Exp parse_sequence(std::string program, std::list<std::string> tail);

/**
 * Determines whether or not a string qualifies as an identifier.
 * @param str The string to check.
 * @return Whether or not the string is an identifier matching (_|[A-Za-z])*
 */
inline bool is_identifier(std::string str) {
    int i;
    for (i = 0; str[i] == '_' || (str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z'); i++);
    return str.length() == i;
}

/**
 * Extracts an identifier from the very beginning of a string.
 * @param str A string possibly containing an identifier.
 * @return An identifier from the beginning of a string; matches (_|[A-Za-z])*.*
 */
inline std::string extract_identifier(std::string str) {
    int i;
    for (i = 0; str[i] == '_' || (str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z'); i++);

    return str.substr(0, i);
}

/**
 * Generates a "null-terminated" list from an STL list.
 * @param vals The list of values to store.
 * @param null The null terminator.
 * @param <T> The type of the expressions in the list.
 * @return A plain list containing the values followed by a null terminator.
 */
template<typename T>
inline T* store_in_list(std::list<T> vals, T null) {
    T *lst = new T[vals.size()+1];
    lst[vals.size()] = null;
    
    int i = 0;
    for (auto it = vals.begin(); it != vals.end(); it++, i++) {
        lst[i] = *it;
    }
    
    return lst;
}

/**
 * Frees all of the elements in a list.
 * @param vals A list of freeable values.
 * @param <T> The type of the elements in the list.
 */
template<typename T>
inline void free_all_in_list(std::list<T> vals) {
    for (auto it = vals.begin(); it != vals.end(); it++)
        delete *it;
}

/**
 * Trims whitespace from the outer edges of a string.
 * @param str The string to change.
 * @return The string minus the bounding whitespace.
 */
inline std::string trim_whitespace(std::string str) {
    int i;
    for (i = 0; str[i] == ' ' || str[i] == '\n' || str[i] == '\t'; i++);

    int j;
    for (j = str.length(); j > i && str[j-1] == ' ' || str[j-1] == '\n' || str[j-1] == '\t'; j--);
    
    return str.substr(i, j-i);
}

/**
 * Determines whether or not a string contains only whitespace.
 * @param str A string to check for whitespace.
 *@return Whether or not the string contains only whitespace.
 */
inline bool is_all_whitespace(std::string str) {
    for (int i = str.length()-1; i >= 0; i--)
        if (str[i] != ' ' && str[i] != '\n' && str[i] != '\t')
            return false;

    return true;
}

#endif
