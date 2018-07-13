#include "parser.hpp"

using namespace std;

int index_of_char(string str, char c) {
    for (int i = 0; i < str.length(); i++) {
        if (str[i] == c)
            return i;
        
        // We must parse for closures.
        int j = 0;
        switch (str[i]) {
            case '(': // Parentheses
                j = index_of_closure(str.substr(i), '(', ')'); break;
            case '{': // Brackets
                j = index_of_closure(str.substr(i), '{', '}'); break;
            case '"': // String literals
                j = index_of_closure(str.substr(i), '"', '"'); break;
            case '[': // Braces
                j = index_of_closure(str.substr(i), '[', ']'); break;
        }

        if (j == -1) {
            // This expression cannot work.
            return -1;
        } else
            i += j;
    }

    return c ? -1 : str.length();
}

int index_of_closure(string str, char a, char b) {
    int i;
    for (i = 1; i < str.length(); i++) {
        // Ignore escaped characters.
        if (str[i] == b) {
            // We have found closure; return the index of b.
            return i;
        } else if (a == b && a == '"') {
            if (str[i] == '\\') {
                i++;
                continue;
            }
        } else {
            int j = 0;
            switch (str[i]) {
                case '(': // Parentheses
                    j = index_of_closure(str.substr(i), '(', ')'); break;
                case '{': // Brackets
                    j = index_of_closure(str.substr(i), '{', '}'); break;
                case '"': // String literals
                    j = index_of_closure(str.substr(i), '"', '"'); break;
                case '[': // Braces
                    j = index_of_closure(str.substr(i), '[', ']'); break;
            }

            if (j == -1)
                return -1;
            else
                i += j;
        }
    }
    
    // Trim the string for the error message
    if (str.find('\n') >= 0)
        str = str.substr(0, str.find('\n'));

    return -1;
}

int starts_with(string str, string lit) {
    int i;
    for (i = 0; str[i] == ' ' || str[i] == '\n' || str[i] == '\t'; i++);

    if (str.substr(i, lit.length()) != lit)
        return -1;
    
    i += lit.length();
    
    // If it is possible to extend the string with an identifier, then it is bad.
    if (is_identifier(lit) && extract_identifier(str.substr(i)) != "")
        return -1;
    
    // Trim more whitespace
    for (;str[i] == ' ' || str[i] == '\n' || str[i] == '\t'; i++);
        
    return i;
}

list<string> extract_statements(string str, char delim, bool trim_empty) {
    list<string> result;

    // We seek to find the next semicolon or evidence that there is
    // exactly one statement in our program.
    int i;
    for (i = 0; i < str.length(); i++) {

        if(str[i] == delim) { // Semicolon implies that we found our result.
            // Extract the statement and hold onto it.
            string cmd = str.substr(0, i);
            result.push_back(cmd);

            // Progress the string
            str = str.substr(i+1);

            // Reset the counter
            i = -1;
            continue;
        }
    
        // We must parse for closures.
        int j = 0;
        switch (str[i]) {
            case '(': // Parentheses
                j = index_of_closure(str.substr(i), '(', ')'); break;
            case '{': // Brackets
                j = index_of_closure(str.substr(i), '{', '}'); break;
            case '"': // String literals
                j = index_of_closure(str.substr(i), '"', '"'); break;
            case '[': // Braces
                j = index_of_closure(str.substr(i), '[', ']'); break;
        }

        if (j == -1) {
            // This expression cannot work.
            result.clear();
            return result;
        } else
            i += j;
    }

    if (i) // There is one more statement to be had.
        result.push_back(str);

    // We do not wish to hold onto strings that are exclusively whitespace.
    if (trim_empty)
        result.remove_if(is_all_whitespace);

    return result;
}

