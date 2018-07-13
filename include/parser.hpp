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

result<Expression> parse_pemdas(std::string str, int = 12);
result<Expression> parse_body(std::string, bool = false);

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
Exp parse_sequence(std::string program, std::list<std::string>);

inline bool is_identifier(std::string str) {
    int i;
    for (i = 0; str[i] == '_' || (str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z'); i++);
    return str.length() == i;
}

inline std::string extract_identifier(std::string str) {
    int i;
    for (i = 0; str[i] == '_' || (str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z'); i++);

    return str.substr(0, i);
}

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
template<typename T>
inline void free_all_in_list(std::list<T> vals) {
    for (auto it = vals.begin(); it != vals.end(); it++)
        delete *it;
}

/**
 * Trims whitespace from the outer edges of a string
 */
inline std::string trim_whitespace(std::string str) {
    int i;
    for (i = 0; str[i] == ' ' || str[i] == '\n' || str[i] == '\t'; i++);

    int j;
    for (j = str.length(); j > i && str[j-1] == ' ' || str[j-1] == '\n' || str[j-1] == '\t'; j--);
    
    return str.substr(i, j-i);
}

inline bool is_all_whitespace(std::string str) {
    for (int i = str.length()-1; i >= 0; i--)
        if (str[i] != ' ' && str[i] != '\n' && str[i] != '\t')
            return false;

    return true;
}

#endif
