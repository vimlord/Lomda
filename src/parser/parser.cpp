#include "parser.hpp"

using namespace std;

#include <vector>

#include <iostream>

/**
 * Given a raw program, remove comments from the code and return the
 * undocumented result.
 *
 * @return The uncommented program on success or an empty string on failure.
 */
string preprocess_program(string str) {
    for (unsigned int i = 0; i < str.length(); i++) {
        if (str[i] == '"') {
            int j = index_of_closure(str.substr(i), '"', '"');
            if (j == -1) return "";
            else i += j;
        } else if (str[i] == '#') {
            // Find the end of line or end of program
            int j = i+1;
            while (str[j] != '\n' && str[j]) j++;
            // Perform trimming of the string
            str = str.substr(0, i) + str.substr(j);
            i--;
        }
    }
    return str;
}


Exp parse_program(string str) {
    str = preprocess_program(str);
    if (is_all_whitespace(str)) {
        throw_err("parser", "program does not contain any executable code");
        return NULL;
    }

    auto statements = extract_statements(str);
    
    // If statements cannot be extracted, we cannot build an expression.
    if (statements.empty()) {
        return NULL;
    } else
        throw_debug("parser", "extracted " + to_string(statements.size()) + " lines from '" + str + "'");

    string first = statements.front();
    statements.pop_front();
    
    Exp program = parse_sequence(first, statements);

    if (!program)
        throw_err("parser", "the given program could not be converted to an AST via BNF parsing");
    else if (configuration.verbosity)
        throw_debug("parsed", program->toString());
    
    return program;
}

