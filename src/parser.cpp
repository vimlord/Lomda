#include "parser.hpp"

using namespace std;

#include <list>
#include <vector>

#include <iostream>

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

result<Expression> parse_pemdas(string str, int = 12);
result<Expression> parse_body(string, bool = false);

bool is_identifier(string str) {
    int i;
    for (i = 0; str[i] == '_' || (str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z'); i++);
    return str.length() == i;
}

string extract_identifier(string str) {
    int i;
    for (i = 0; str[i] == '_' || (str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z'); i++);

    return str.substr(0, i);
}

template<typename T>
T* store_in_list(list<T> vals, T null) {
    T *lst = new T[vals.size()+1];
    lst[vals.size()] = null;
    
    int i = 0;
    for (auto it = vals.begin(); it != vals.end(); it++, i++) {
        lst[i] = *it;
    }
    
    return lst;
}
template<typename T>
void free_all_in_list(list<T> vals) {
    for (auto it = vals.begin(); it != vals.end(); it++)
        delete *it;
}

/**
 * Given a pairing of opening/closing characters, find the index of closure.
 *
 * @param str A string of the form aSbS, S not necessarily equivalent.
 * @param a A recognized opening character.
 * @param b A recognized closing character.
 * @return The index of the closing b, or -1 if not closed.
 */
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

    string mssg = "missing closing '";
    mssg += b;
    mssg += "' in\n\t" + str.substr(0, 16) + "\n\t^";
    throw_err("parser", mssg); 

    return -1;
}

/**
 * Trims whitespace from the outer edges of a string
 */
string trim_whitespace(string str) {
    int i;
    for (i = 0; str[i] == ' ' || str[i] == '\n' || str[i] == '\t'; i++);

    int j;
    for (j = str.length(); j > i && str[j-1] == ' ' || str[j-1] == '\n' || str[j-1] == '\t'; j--);
    
    return str.substr(i, j-i);
}

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

bool is_all_whitespace(string str) {
    for (int i = str.length()-1; i >= 0; i--)
        if (str[i] != ' ' && str[i] != '\n' && str[i] != '\t')
            return false;

    return true;
}

/**
 * Given a string, returns the amount of characters to trim to traverse
 * over the literal and clear whitespace, or -1 if not present.
 */
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

/**
 * We know that a program is a series of expressions split by semicolons.
 * Thus, we seek to perform tokenization split by semicolons.
 */
list<string> extract_statements(string str, char delim = ';', bool trim_empty = true) {
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



result<Type> parse_type(string str) {
    result<Type> res;
    res.value = NULL;
    res.strlen = -1;

    int i;
    if ((i = starts_with(str, "Z")) > 0)
        res.value = new IntType;
    else if ((i = starts_with(str, "R")) > 0)
        res.value = new RealType;
    else if ((i = starts_with(str, "S")) > 0)
        res.value = new StringType;
    else if ((i = starts_with(str, "V")) > 0)
        res.value = new VoidType;
    else if ((i = starts_with(str, "ADT")) > 0) {
        int j;

        // The opening brace
        if ((j = starts_with(str.substr(i), "<")) == -1) {
            for (i = 0; str[i] == ' ' || str[i] == '\n' || str[i] == '\t'; i++);
            str = str.substr(i);
            if (str.find('\n') > 0)
                str = str.substr(0, str.find('\n'));
            throw_err("parser", "closing less-than sign expected, but not found; see:\n\t" + str.substr(0, 16));

            // No type could be found
            return res;
        } else
            i += j;
        
        // The name of the ADT
        string id = extract_identifier(str.substr(i));
        if (id == "") {
            for (i = 0; str[i] == ' ' || str[i] == '\n' || str[i] == '\t'; i++);
            str = str.substr(i);
            if (str.find('\n') > 0)
                str = str.substr(0, str.find('\n'));
            throw_err("parser", "expected identifier, but not found; see:\n\t" + str.substr(0, 16));

            // No type could be found
            return res;
        } else
            i += id.length();
        
        // The closing brace
        if ((j = starts_with(str.substr(i), ">")) == -1) {
            for (i = 0; str[i] == ' ' || str[i] == '\n' || str[i] == '\t'; i++);
            str = str.substr(i);
            if (str.find('\n') > 0)
                str = str.substr(0, str.find('\n'));
            throw_err("parser", "closing greater-than sign expected, but not found; see:\n\t" + str.substr(0, 16));

            // No type could be found
            return res;
        } else {
            // We can now define the type
            i += j;
            res.value = new AlgebraicDataType(id);
        }

    } else if ((i = starts_with(str, "(")) > 0) {
        // Encapsulate a type in parentheses.
        i = index_of_char(str, '(');
        int j = index_of_closure(str.substr(i), '(', ')');
        
        if (j == -1)
            return res;

        res = parse_type(str.substr(i+1, j-1));
        if (!res.value) {
            res.reset();
            return res;
        } else
            i += j+1;
    } else if ((i = starts_with(str, "[")) > 0) {
        i = index_of_char(str, '[');
        int j = index_of_closure(str.substr(i), '[', ']');

        if (j == -1)
            return res;

        res = parse_type(str.substr(i+1, j-1));
        if (!res.value) {
            res.reset();
            return res;
        } else {
            res.value = new ListType(res.value);
            i += j+1;
        }
        
    } else {
        // Print an error message designating that a problem occurred.
        for (i = 0; str[i] == ' ' || str[i] == '\n' || str[i] == '\t'; i++);
        str = str.substr(i);
        if (str.find('\n') > 0)
            str = str.substr(0, str.find('\n'));
        throw_err("parser", "unrecognized type detected; see:\n\t" + str.substr(0, 16));

        // No type could be found
        return res;
    }

    // Progress the string
    str = str.substr(i);

    // Adjust the length of the string to compensate.
    res.strlen = i;

    // Now, we will check to see if there is more.
    if ((i = starts_with(str, "->")) > 0) {
        result<Type> alt = parse_type(str.substr(i));
        if (!alt.value)
            res.reset();
        else {
            res.value = new LambdaType(res.value, alt.value);
            res.strlen += i + alt.strlen;
        }
    } else if ((i = starts_with(str, "*")) > 0) {
        result<Type> alt = parse_type(str.substr(i));
        if (!alt.value)
            res.reset();
        else {
            res.value = new TupleType(res.value, alt.value);
            res.strlen += i + alt.strlen;
        }
    }

    return res;

}

result<Expression> parse_stdmathfn(string str, StdMathExp::MathFn fn) {
    result<Expression> base;
    auto arg = parse_pemdas(str, 1);
    if (arg.value) {
        base.value = new StdMathExp(fn, arg.value);
        base.strlen = arg.strlen;
    } else
        base.reset();

    return base;
}

/**
 * Using order of operations, parse the string for an expression.
 * str   - The string to extract a PEMDAS operation from
 * order - The location in the hierarchy to search at
 */
result<Expression> parse_pemdas(string str, int order) {

    result<Expression> base;
    base.value = NULL;
    base.strlen = -1;
    
    // We will first attempt to derive a unary expression
    // from the front of the expression.
    if (order >= 3) {
        int i;
        if ((i = starts_with(str, "not")) > 0) {
            // Not gate
            base = parse_pemdas(str.substr(i), 2);
            if (base.value) {
                base.value = new NotExp(base.value);
                base.strlen += i;
            } else {
                return base;
            }
        } else if ((i = starts_with(str, "-")) > 0) {
            // Negation
            base = parse_pemdas(str.substr(i), 2);
            if (base.value) {
                base.value = new MultExp(new IntExp(-1), base.value);
                base.strlen += i;
            } else
                return base;
        }
        
        // If we found a unary expression, progress the string
        if (base.strlen > 0) {
            str = str.substr(base.strlen);
        }
    }

    // If we cannot build a unary expression, then we simply
    // build a primitive expression.
    if (base.strlen == -1) {
        int len;
        if ((len = starts_with(str, "(")) > 0) {

            // It is a parenthesized expression
            len = index_of_char(str, '(');

            int j = index_of_closure(str.substr(len), '(', ')');
            if (j == -1) {
                // Closure is not maintained.
                base.reset();
                return base;
            }
            string seg = str.substr(len+1, j-1);

            // Set an initial hypothesis for the length
            base.strlen = len + j + 1;
            str = str.substr(base.strlen);
            
            if ((j = starts_with(str, "->")) > 0) {
                // Anonymous declaration of the lambda.
                // The length is a bit longer.
                base.strlen += j;
                str = str.substr(j);
                
                // Parse the arguments
                list<string> argv;
                if (!is_all_whitespace(seg)) while (true) {
                    // Push up to the front
                    int i;
                    for (i = 0; seg[i] == ' ' || seg[i] == '\n' || seg[i] == '\t'; i++);
                    seg = seg.substr(i);

                    string id = extract_identifier(seg);
                    if (id == "") {
                        // The content is not an argument.
                        base.reset();
                        return base;
                    } else {
                        // Apply the argument
                        argv.push_back(id);
                        seg = seg.substr(id.length());
                    }

                    if ((j = starts_with(seg, ",")) > 0) {
                        // Another argument
                        seg = seg.substr(j);
                    } else if (is_all_whitespace(seg))
                        // End of the list
                        break;
                    else {
                        // There is noise
                        base.reset();
                        return base;
                    }
                }

                // Store the elements into a list
                string *args = store_in_list<string>(argv, "");

                // Thus, we parse for a body.
                result<Expression> body = parse_body(str);
                if (!body.value) {
                    base.reset();
                    return base;
                }

                base.value = new LambdaExp(args, body.value);
                base.strlen += body.strlen;

            } else {
                // Parse the contents
                base.value = parse_pemdas(seg).value;
                if (!base.value) {
                    base.reset();
                    return base;
                }
            }

        } else if ((len = starts_with(str, "[")) > 0) {
            int j = index_of_closure(str.substr(len-1), '[', ']');
            if (j == -1) {
                base.reset();
                return base;
            }

            // Extract an argument list
            string seg = str.substr(len, j - 1);

            // Progress the string
            base.strlen = len + j;
            str = str.substr(base.strlen);
            
            list<string> argv = extract_statements(seg, ',', false);

            // Build the argument list if possible.
            auto args = new ArrayList<Exp>;
            base.value = new ListExp(args);

            int i = 0;
            for (auto it = argv.begin(); it != argv.end(); it++, i++) {
                // Attempt to extract the item
                string x = *it;
                result<Expression> arg = parse_pemdas(x);
                if (!arg.value || !is_all_whitespace(x.substr(arg.strlen))) {
                    base.reset();
                    return base;
                } else
                    args->add(i, arg.value);
            }

        } else if ((len = starts_with(str, "{")) > 0) {
            len = index_of_char(str, '{');
            int j = index_of_closure(str.substr(len), '{', '}');
            if (j == -1) {
                base.reset();
                return base;
            }

            // Extract an argument list
            string seg = str.substr(len + 1, j - 1);

            // Progress the string
            base.strlen = len + j + 1;
            str = str.substr(base.strlen);
            
            list<string> argv = extract_statements(seg, ',', false);
            
            // Initial condition: nothing
            auto keys = new LinkedList<string>;
            auto vals = new LinkedList<Exp>;
            base.value = new DictExp(keys, vals);

            int i = 0;
            while (argv.size()) {
                // Attempt to extract the item
                string pair = argv.back();
                argv.pop_back();
                
                // Trim the spaces
                int i;
                for (i = 0; pair[i] == ' ' || pair[i] == '\n' || pair[i] == '\t'; i++);
                pair = pair.substr(i);

                string x = extract_identifier(pair);
                if (x == "") {
                    base.reset();
                    return base;
                }
                
                pair = pair.substr(x.length());

                if ((i = starts_with(pair, ":")) == -1) {
                    base.reset();
                    return base;
                } else
                    pair = pair.substr(i);
                
                result<Expression> arg = parse_pemdas(pair);
                if (arg.value && is_all_whitespace(pair.substr(arg.strlen))) {
                    // We can provide another item
                    keys->add(0, x);
                    vals->add(0, arg.value);
                } else {
                    base.reset();
                    return base;
                }

            }

        } else if ((len = starts_with(str, "\"")) > 0) {
            len = index_of_char(str, '"');
            int j = index_of_closure(str.substr(len), '"', '"');
            if (j == -1) {
                base.reset();
                return base;
            } else
                base.strlen = len + j + 1;
            
            // Process the string literal by handling escape characters.
            string literal = str.substr(len+1, j-1);
            str = str.substr(base.strlen);

            vector<char> strbin;
            for (int i = 0; i < literal.length(); i++) {
                if (literal[i] == '\\') {
                    i++;
                    switch (literal[i]) {
                        case '\0':
                            // That should not be possible.
                            base.reset();
                            return base;
                        case 'n':
                            strbin.push_back('\n');
                            break;
                        case 't':
                            strbin.push_back('\t');
                            break;
                        default:
                            strbin.push_back(literal[i]);
                    }
                } else
                    strbin.push_back(literal[i]);
            }

            // Provide the null terminator of the string.
            strbin.push_back('\0');
            
            base.value = new StringExp(string(&strbin[0]));

        } else {
            // Parse an absolute primitive
            len = 0;
            while (str[len] == ' ' || str[len] == '\n' || str[len] == '\t')
                len++;
            
            // Progbases the string
            str = str.substr(len);
            
            // Attempt to extract a variable.
            string var = extract_identifier(str);
            if (var != "") {
                // Apply to the length of the segment
                base.strlen = var.length() + len;

                // Progress the string
                str = str.substr(var.length());

                // The identifier could be a baseerved constant or a custom var.
                if (var == "true")
                    base.value = new TrueExp;
                else if (var == "false")
                    base.value = new FalseExp;
                else if (var == "void")
                    base.value = new VoidExp;
                else if (var == "sin") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::SIN);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "cos") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::COS);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "tan") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::TAN);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "log") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::LOG);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "sqrt") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::SQRT);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "exp") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::EXP);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "max") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::MAX);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "min") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::MIN);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "lambda") {
                    // Lambda declared by keyword.
                    
                    // Parse the opening
                    int i = starts_with(str, "(");
                    if (i == -1) {
                        base.reset();
                        return base;
                    } else {
                        i = index_of_char(str, '(');
                        str = str.substr(i);
                        base.strlen += i;
                    }
                    
                    // Parse the closure
                    int j = index_of_closure(str, '(', ')');
                    if (j == -1) {
                        base.reset();
                        return base;
                    } else
                        base.strlen += j + 1;

                    // Extract the argument list
                    string seg = str.substr(1, j-1);
                    str = str.substr(j+1);

                    // Parse the arguments
                    list<string> argv;
                    if (!is_all_whitespace(seg)) while (true) {
                        // Push up to the front
                        int i;
                        for (i = 0; seg[i] == ' ' || seg[i] == '\n' || seg[i] == '\t'; i++);
                        seg = seg.substr(i);

                        string id = extract_identifier(seg);
                        if (id == "") {
                            // The content is not an argument.
                            base.reset();
                            return base;
                        } else {
                            // Apply the argument
                            argv.push_back(id);
                            seg = seg.substr(id.length());
                        }

                        if ((j = starts_with(seg, ",")) > 0) {
                            // Another argument
                            seg = seg.substr(j);
                        } else if (is_all_whitespace(seg))
                            // End of the list
                            break;
                        else {
                            // There is noise
                            base.reset();
                            return base;
                        }
                    }

                    // Store the elements into a list
                    string *args = store_in_list<string>(argv, "");

                    // Thus, we parse for the body.
                    result<Expression> body = parse_body(str);
                    if (!body.value) {
                        base.reset();
                        return base;
                    }
                    
                    // Create the end result
                    base.value = new LambdaExp(args, body.value);
                    base.strlen += body.strlen;

                } else if (var == "thunk") {
                    result<Expression> next = parse_body(str, false);
                    
                    if (!next.value) {
                        base.reset();
                        return base;
                    }
                    
                    // The thunk is legal; we will keep it
                    base.value = new ThunkExp(next.value);
                    base.strlen += next.strlen;
                    str = str.substr(next.strlen);

                } else
                    base.value = new VarExp(var);

                if (!base.value)
                    return base;

            } else {
            
                // Scratch space for parsing numbers
                string::size_type Zlen = -1;
                string::size_type Rlen = -1;
                int Z;
                float R;

                // Attempt to extract a real
                try { R = stof(str, &Rlen); }
                catch (std::out_of_range oor) { Rlen = -1; }
                catch (std::invalid_argument ia) { Rlen = -1; }

                // Attempt to extract an integer
                try { Z = stoi(str, &Zlen, 10); }
                catch (std::out_of_range oor) { Zlen = -1; }
                catch (std::invalid_argument ia) { Zlen = -1; }
                
                // If one of them prevails, choose it.
                if (Rlen > Zlen) {
                    base.value = new RealExp(R);
                    base.strlen = Rlen + len;
                    str = str.substr(Rlen);
                } else if (Zlen > Rlen || (int) Zlen > 0) {
                    base.value = new IntExp(Z);
                    base.strlen = Zlen + len;
                    str = str.substr(Zlen);
                } else {
                    // In any other case, there is no possible primitive.
                    // Hence, this arrangement is impossible.
                    if (str.find('\n') >= 0)
                        str = str.substr(0, str.find('\n'));
                    
                    string under = "";
                    for (int i = 0; i < 16 && i < str.length(); i++)
                        if (str[i] != ' ' && str[i] != '\n' && str[i] != '\t')
                            under += "^";
                        else
                            break;

                    throw_err("parser", "non-primitive symbol found when primitive expected; see:\n\t'" + str.substr(0, 16) + "'\n\t " + under);

                    base.reset();
                    return base;
                }
            }
        }
    }
    
    if (order >= 1) {
        // Array access, function calls, parentheses
        while (base.value) {
            int i;
            if ((i = starts_with(str,"(")) > 0) {
                i = index_of_char(str, '(');
                int j = index_of_closure(str.substr(i), '(', ')');
                if (j == 0) {
                    // The parentheses have no closure
                    base.reset();
                    return base;
                } else {
                    base.strlen += i + j + 1;
                }
                
                // Extract a list from which a set of values can be computed.
                string argstr = str.substr(i+1, j-1);
                list<string> argv = extract_statements(argstr, ',');

                list<Exp> args;
                for (auto it = argv.begin(); it != argv.end(); it++) {
                    string arg = *it;
                    result<Expression> e = parse_pemdas(arg);
                    if (!e.value) {
                        // Garbage collect everything
                        for (auto jt = args.begin(); jt != args.end(); jt++)
                            delete *jt;

                        base.reset();

                        return base;
                    } else {
                        args.push_back(e.value);
                    }
                }

                // Progress the string
                str = str.substr(i+j+1);

                // Now, we build our argument set
                auto xs = new Exp[args.size()+1];
                xs[args.size()] = NULL;
                
                for (int i = args.size()-1; i >= 0; i--) {
                    xs[i] = args.back();
                    args.pop_back();
                }
                
                // Build the apply expression and move on.
                base.value = new ApplyExp(base.value, xs);

            } else if ((i = starts_with(str,"[")) > 0) {
                // Array access
                i = index_of_char(str, '[');
                int j = index_of_closure(str.substr(i), '[', ']');
                if (j == -1) {
                    // The brackets have no closure
                    base.reset();
                    return base;
                } else
                    base.strlen += i + j + 1;

                string argstr = str.substr(i+1, j-1);
                str = str.substr(i+j+1);

                // Now, we parse for a pemdas expression
                result<Expression> idx = parse_pemdas(argstr);
                if (!idx.value) {
                    base.reset();
                    return base;
                }

                argstr = argstr.substr(idx.strlen);

                if (is_all_whitespace(argstr)) {
                    // It is a simple accessor
                    base.value = new ListAccessExp(base.value, idx.value);
                } else if ((i = starts_with(argstr, ":")) > 0) {
                    argstr = argstr.substr(i);
                    
                    // Parse the other half
                    result<Expression> jdx = parse_pemdas(argstr);
                    if (!jdx.value) {
                        idx.reset();
                        base.reset();
                        return base;
                    } else if (is_all_whitespace(argstr.substr(jdx.strlen)))
                        base.value = new ListSliceExp(base.value, idx.value, jdx.value);
                    else {
                        // There is noise.
                        idx.reset();
                        jdx.reset();
                        base.reset();
                        return base;
                    }

                } else {
                    // There is noise.
                    idx.reset();
                    base.reset();
                    return base;
                }

            } else if ((i = starts_with(str,".")) > 0) {
                string idx = extract_identifier(str.substr(i)); 

                if (idx == "") {
                    // Tidy up the string so that we can formally complain
                    for (i = 0; str[i] == ' ' || str[i] == '\n' || str[i] == '\t'; i++);
                    str = str.substr(i);
                    if (str.find('\n') > 0)
                        str = str.substr(0, str.find('\n'));

                    throw_err("parser", "dot accessor expects identifier; see:\n\t" + str.substr(0, 16));

                    base.reset();
                    return base;
                } else {
                    base.value = new DictAccessExp(base.value, idx);
                    base.strlen += i + idx.length();
                    str = str.substr(i + idx.length());
                }
            } else
                break;
        }
    }

    if (order >= 2) {
        // Exponentiation
        int i;
        if ((i = starts_with(str, "^")) > 0) {
            str = str.substr(i);
            result<Expression> next = parse_pemdas(str, 2);

            if (next.value) {
                base.value = new ExponentExp(base.value, next.value);
                base.strlen += i + next.strlen;
            } else {
                base.reset();
                return base;
            }
        }
    }

    if (order >= 4) {
        // Membership, type checking, casting

        // Check for a symbol
        int i;
        if ((i = starts_with(str, "in")) > 0) {
            str = str.substr(i);

            result<Expression> next = parse_pemdas(str, 3);
            if (next.value) {
                // Extend the result.
                base.value = new HasExp(base.value, next.value);
                base.strlen += i + next.strlen;
                
                // Progress the string.
                str = str.substr(next.strlen);
            } else {
                // Parsing failed!
                base.reset();
                return base;
            }
        } else {
            // If we are operating on types, seek a keyword to indicate this.
            int op;
            if ((i = starts_with(str, "isa")) > 0)
                op = 0;
            else if ((i = starts_with(str, "as")) > 0)
                op = 1;
            else
                op = -1;
            
            if (op >= 0) {
                str = str.substr(i);
                
                // Extract the type
                result<Type> T = parse_type(str);
                if (!T.value) {
                    // A type could not be extracted
                    base.reset();
                    return base;
                } else {
                    // Build the result
                    base.value = op == 0
                        ? (Exp) new IsaExp(base.value, T.value)
                        : (Exp) new CastExp(T.value, base.value);
                    base.strlen += i + T.strlen;
                }

                str = str.substr(T.strlen);
            }
        }
    }


    if (order >= 5) {
        while (base.value) {
            // Check for a symbol
            int op;

            int i;
            if ((i = starts_with(str, "*")) > 0) {
                op = 0;
            } else if ((i = starts_with(str, "/")) > 0) {
                op = 1;
            } else if ((i = starts_with(str, "mod")) > 0) {
                op = 2;
            } else
                break;

            str = str.substr(i);

            result<Expression> next = parse_pemdas(str, 4);
            if (next.value) {
                // Extend the result.
                base.value = op == 0
                    ? (Exp) new MultExp(base.value, next.value)
                    : op == 1
                    ? (Exp) new DivExp(base.value, next.value)
                    : (Exp) new ModulusExp(base.value, next.value);
                base.strlen += i + next.strlen;
                
                // Progress the string.
                str = str.substr(next.strlen);
            } else {
                // Parsing failed!
                base.reset();
                return base;
            }
        }

    }

    
    if (order >= 6) {
        // Additive operations
        while (base.value) {
            // Check for a symbol
            int op;

            int i;
            if ((i = starts_with(str, "+")) > 0) {
                op = 0;
            } else if ((i = starts_with(str, "-")) > 0) {
                op = 1;
            } else
                break;

            str = str.substr(i);

            result<Expression> next = parse_pemdas(str, 5);
            if (next.value) {
                // Extend the result.
                base.value = op == 0
                    ? (Exp) new SumExp(base.value, next.value)
                    : (Exp) new DiffExp(base.value, next.value);
                base.strlen += i + next.strlen;
                
                // Progress the string.
                str = str.substr(next.strlen);
            } else {
                // Parsing failed!
                base.reset();
                return base;
            }
        }

    }

    if (order >= 7) {
        // Lower comparators
        int i;
        CompOp op;

        if ((i = starts_with(str, ">=")) > 0)
            op = GEQ;
        else if ((i = starts_with(str, "<=")) > 0)
            op = LEQ;
        else if ((i = starts_with(str, ">")) > 0)
            op = GT;
        else if ((i = starts_with(str, "<")) > 0)
            op = LT;
        
        if (i > 0) {
            str = str.substr(i);
            result<Expression> alt = parse_pemdas(str, 6);
            if (!alt.value) {
                base.reset();
                return base;
            }
            
            base.value = new CompareExp(base.value, alt.value, op);
            base.strlen += i + alt.strlen;
        }
    } 
    
    if (order >= 8) {
        // Upper comparators
        int i;
        CompOp op;

        if (
            (i = starts_with(str, "==")) > 0
            ||
            (i = starts_with(str, "is")) > 0
            ||
            (i = starts_with(str, "equals")) > 0
        )
            op = EQ;
        else if ((i = starts_with(str, "!=")) > 0)
            op = NEQ;
        
        if (i > 0) {
            str = str.substr(i);
            result<Expression> alt = parse_pemdas(str, 7);
            if (!alt.value) {
                base.reset();
                return base;
            }
            
            base.value = new CompareExp(base.value, alt.value, op);
            base.strlen += i + alt.strlen;
        }
    } 
    
    if (order >= 9) {
        // AND gate
        while (base.value) {
            // Check for a symbol
            int op;

            int i;
            if ((i = starts_with(str, "and")) == -1)
                break;

            str = str.substr(i);

            result<Expression> next = parse_pemdas(str, 8);
            if (next.value) {
                // Extend the result<Expression>.
                base.value = new AndExp(base.value, next.value);
                base.strlen += i + next.strlen;
                
                // Progress the string.
                str = str.substr(next.strlen);
            } else {
                // Parsing failed!
                base.reset();
                return base;
            }
        }
    }
    
    if (order >= 10) {
        // OR gate
        while (base.value) {
            // Check for a symbol
            int op;

            int i;
            if ((i = starts_with(str, "or")) == -1)
                break;

            str = str.substr(i);

            result<Expression> next = parse_pemdas(str, 9);
            if (next.value) {
                // Extend the result.
                base.value = new OrExp(base.value, next.value);
                base.strlen += i + next.strlen;
                
                // Progress the string.
                str = str.substr(next.strlen);
            } else {
                // Parsing failed!
                base.reset();
                return base;
            }
        }
    }
    
    if (order >= 11) {
        // Assignment
        int i;
        if ((i = starts_with(str, "=")) > 0) {
            result<Expression> next = parse_pemdas(str.substr(i), 10);
            if (!next.value) {
                base.reset();
                return base;
            }

            base.value = new SetExp(base.value, next.value);
            base.strlen += i + next.strlen;
            
            str = str.substr(i + next.strlen);
        }
    }

    if (order >= 12) {
        int i;
        if ((i = starts_with(str, ",")) > 0) {
            result<Expression> next = parse_pemdas(str.substr(i), 11);
            if (!next.value) {
                base.reset();
                return base;
            }

            base.value = new TupleExp(base.value, next.value);
            base.strlen += i + next.strlen;

            str = str.substr(i + next.strlen);
        }
    }

    return base;

}

result<Expression> parse_body(string str, bool terminates) {
    if (starts_with(str, "{") > 0) {
        // We seek to parse a body that contains a program.
        int i = index_of_char(str, '{');
        int j = index_of_closure(str.substr(i), '{', '}');
        
        result<Expression> res;

        if (terminates && !is_all_whitespace(str.substr(i+j+1))) {
            // The body is supposed to encompass the entire string
            res.reset();
            return res;
        }

        res.value = parse_program(str.substr(i+1, j-1));
        if (!res.value)
            res.strlen = -1;
        else
            res.strlen = i+j + 1;

        return res;

    } else {
        // We seek to compute a PEMDAS expression.
        result<Expression> res = parse_pemdas(str);
        if (res.value && terminates && !is_all_whitespace(str.substr(res.strlen))) {
            res.reset();
        }
        
        return res;
    }

}

Exp parse_statement(string str) {
    int i;

    if ((i = starts_with(str, "while")) > 0) {
        str = str.substr(i);

        // Parse a condition.
        result<Expression> cond = parse_pemdas(str);
        if (!cond.value)
            return NULL;
        else
            // Process the string further.
            str = str.substr(cond.strlen);

        // Compute the body.
        result<Expression> body = parse_body(str, true);
        if (!body.value) {
            delete cond.value;
            return NULL;
        }

        return new WhileExp(cond.value, body.value);
       
    } else if ((i = starts_with(str, "if")) > 0) {
        str = str.substr(i);
        
        // Parse the conditional
        result<Expression> cond = parse_pemdas(str);
        if (!cond.value)
            return NULL;
        else
            str = str.substr(cond.strlen);
        
        // Then keyword
        if ((i = starts_with(str, "then")) < 0) {
            if (str.find('\n') >= 0)
                str = str.substr(0, str.find('\n'));
            throw_err("parser", "if statement should include then keyword; see:\n\t" + str.substr(0, 16));

            delete cond.value;
            return NULL;
        } else
            str = str.substr(i);
        
        // Parse the first body
        result<Expression> tBody = parse_body(str);
        if (!tBody.value) {
            delete cond.value;
            return NULL;
        } else
            str = str.substr(tBody.strlen);

        // Else keyword
        if ((i = starts_with(str, "else")) < 0) {
            if (str.find('\n') >= 0)
                str = str.substr(0, str.find('\n'));
            throw_err("parser", "if statement missing else keyword; see:\n\t" + str.substr(0, 16));

            delete cond.value;
            delete tBody.value;
            return NULL;
        } else
            str = str.substr(i);

        // Parse the second body
        result<Expression> fBody = parse_body(str, true);
        if (!fBody.value) {
            delete cond.value;
            delete tBody.value;
            return NULL;
        }

        // Build the end result
        return new IfExp(cond.value, tBody.value, fBody.value);
    } else if ((i = starts_with(str, "fold")) > 0) {
        str = str.substr(i);
        
        // Parse the lstitional
        result<Expression> lst = parse_pemdas(str);
        if (!lst.value)
            return NULL;
        else
            str = str.substr(lst.strlen);
        
        // Then keyword
        if ((i = starts_with(str, "into")) < 0) {
            if (str.find('\n') >= 0)
                str = str.substr(0, str.find('\n'));
            throw_err("parser", "fold statement missing into keyword; see:\n\t" + str.substr(0, 16));
            delete lst.value;
            return NULL;
        } else
            str = str.substr(i);
        
        // Parse the first body
        result<Expression> func = parse_body(str);
        if (!func.value) {
            delete lst.value;
            return NULL;
        } else
            str = str.substr(func.strlen);

        // Else keyword
        if ((i = starts_with(str, "from")) < 0) {
            if (str.find('\n') >= 0)
                str = str.substr(0, str.find('\n'));
            throw_err("parser", "fold statement missing from keyword; see:\n\t" + str.substr(0, 16));
            delete lst.value;
            delete func.value;
            return NULL;
        } else
            str = str.substr(i);

        // Parse the selst body
        result<Expression> init = parse_body(str, true);
        if (!init.value) {
            delete lst.value;
            delete func.value;
            return NULL;
        }

        // Build the end result
        return new FoldExp(lst.value, func.value, init.value);
    } else if ((i = starts_with(str, "map")) > 0) {
        str = str.substr(i);
        
        // Parse the conditional
        result<Expression> func = parse_pemdas(str);
        if (!func.value)
            return NULL;
        else
            str = str.substr(func.strlen);
        
        // Then keyword
        if ((i = starts_with(str, "over")) < 0) {
            if (str.find('\n') >= 0)
                str = str.substr(0, str.find('\n'));
            throw_err("parser", "map statement missing from keyword; see:\n\t" + str.substr(0, 16));
            delete func.value;
            return NULL;
        } else
            str = str.substr(i);
        
        // Parse the first body
        result<Expression> lst = parse_body(str);
        if (!lst.value) {
            delete func.value;
            return NULL;
        } else
            str = str.substr(lst.strlen);

        // Build the end result
        return new MapExp(func.value, lst.value);

    } else if ((i = starts_with(str, "for")) > 0) {
        // Progress to the potential identifier.
        while (str[i] == ' ' || str[i] == '\n' || str[i] == '\t')
            i++;
        str = str.substr(i);
        
        // Get the identifier.
        string id = extract_identifier(str);
        str = str.substr(id.length());
        
        // Check the identifier condition
        if ((i = starts_with(str, "in")) < 0)
            return NULL;
        else
            str = str.substr(i);

        // Derive the iterable
        result<Expression> lst = parse_pemdas(str);
        if (!lst.value)
            return NULL;
        else
            str = str.substr(lst.strlen);

        // Derive the body
        result<Expression> body = parse_body(str, true);
        if (!body.value) {
            delete lst.value;
            return NULL;
        }
        
        // Now, we can finalize the statement
        return new ForExp(id, lst.value, body.value);

    } else if ((i = starts_with(str, "print")) > 0) {
        // Extract arguments
        list<string> argv = extract_statements(str.substr(i), ',', false);
        list<Exp> args;
        for (auto it = argv.begin(); it != argv.end(); it++) {
            result<Expression> e = parse_pemdas(*it);
            if (!e.value) {
                for (auto jt = args.begin(); jt != args.end(); jt++)
                    delete *jt;
                return NULL;
            } else
                args.push_back(e.value);
        }
        
        // Store the items
        Exp *items = store_in_list<Exp>(args, NULL);

        return new PrintExp(items);
        
    } else if ((i = starts_with(str, "d/d")) > 0) {
        // Attempt to extract the variable
        string x = extract_identifier(str.substr(index_of_char(str, 'd') + 3));

        if (x == "") {
            if (str.find('\n') >= 0)
                str = str.substr(index_of_char(str, 'd'), str.find('\n'));
            throw_err("parser", "differential must include attached identifier; see:\n\t" + str.substr(0, 16));
            return NULL;
        }
        
        str = str.substr(i + x.length());
        
        Exp Y = parse_statement(str);
        if (Y)
            Y = new DerivativeExp(Y, x);

        return Y;

    } else if ((i = starts_with(str, "switch")) > 0) {
        str = str.substr(i);
        
        // Extract an ADT
        result<Expression> adt = parse_pemdas(str, 1); // Identification is limited to low level definitions
        if (!adt.value) return NULL;
        else str = str.substr(adt.strlen);

        if ((i = starts_with(str, "in")) == -1) {
            throw_err("parser", "switch statement missing in keyword; see:\n\t" + str.substr(0, 16));
            delete adt.value;
            return NULL;
        } else
            str = str.substr(i);
        
        // Extract a statement list
        list<string> states = extract_statements(str, '|', false);
        
        // Build argument sets
        list<string> names;
        list<string*> idss;
        list<Exp> bodies;

        while (states.size()) {
            // Parse the front
            string state = states.front();
            
            state = trim_whitespace(state);
            
            // Parse the name and then trim following whitespace
            string name = extract_identifier(state);
            if (name == "") break;
            else state = trim_whitespace(state.substr(name.length()));
            
            // Record the kind.
            names.push_back(name);
            
            if (state[0] != '(') break;
            
            // Extract arguments
            int j = index_of_closure(state, '(', ')');
            if (j == -1) break;
            string argstr = state.substr(1, j-1);
            state = state.substr(j+1);
            
            if (!is_all_whitespace(argstr)) {
                // Parse through the arguments; they should all be identifiers.
                list<string> args = extract_statements(argstr, ',', false);
                string *ids = new string[args.size()+1];
                ids[args.size()] = "";
                for (i = 0; args.size(); i++) {
                    // Pull out and scrape the string down
                    string id = trim_whitespace(args.front());
                    
                    // Check to see that the entire string is an identifier.
                    if (id.length() == 0 || extract_identifier(id) != id) break;
                    else ids[i] = id;

                    args.pop_front();
                }
                
                // Proper or not, add to the list of arguments.
                idss.push_back(ids);
                
                // If some items were not processed, parsing failed.
                if (args.size()) break;
            } else {
                // Establish the empty case
                string *ids = new string[1];
                ids[0] = "";
                idss.push_back(ids);
            }

            // Make sure the arrow can be parsed.
            if ((i = starts_with(state, "->")) < 0) break;
            else state = state.substr(i);
            
            // Parse the body.
            result<Expression> body = parse_body(state, true);
            if (!body.value) break;
            else bodies.push_back(body.value);

            // Mark as complete
            states.pop_front();
        }

        if (states.size()) {
            throw_err("parser", "switch bodies could not be parsed");
            // The evaluation failed
            delete adt.value;
            while (bodies.size()) { delete bodies.front(); bodies.pop_front(); }
            while (idss.size()) { delete[] idss.front(); idss.pop_front(); }
            return NULL;
        }

        return new SwitchExp(
                adt.value,
                store_in_list<string>(names, ""),
                store_in_list<string*>(idss, NULL),
                store_in_list<Exp>(bodies, NULL)
        );


    } else {
        // End case is that we parse for a pemdas expression.
        result<Expression> res = parse_pemdas(str);
        if (res.value && !is_all_whitespace(str.substr(res.strlen))) {
            delete res.value;
            return NULL;
        } else
            return res.value;
    }
}

Exp parse_sequence(string str, list<string> future) {
    int i;

    if ((i = starts_with(str, "let")) > 0) {
        // Let expression
        if (future.size() == 0) {
            if (str.find('\n') >= 0)
                str = str.substr(0, str.find('\n'));

            throw_err("parser", "let statement must be followed by a valid sequence:\n\t" + str.substr(0, 16));

            return NULL;
        }

        // Parse the argument list
        auto args = extract_statements(str.substr(i), ',', false);
        if (args.size() == 0) {
            if (str.find('\n') >= 0)
                str = str.substr(0, str.find('\n'));

            throw_err("parser", "let statement must define at least one variable:\n\t" + str.substr(0, 16));
            return NULL;
        }
        
        // Three sets of values to track.
        list<string> ids;
        list<Exp> vals;
        list<bool> recs;

        for (auto p = args.begin(); p != args.end(); p++) {
            // We should be able to say that something equals something.
            int i = index_of_char(*p, '=');
            string nam = p->substr(0, i);
            string val = p->substr(i+1);
 
            // Make an attempt to parse the expression on the right side.
            // Exactly nothing should be left over during the search.
            auto exp = parse_pemdas(val);
            if (!exp.value || !is_all_whitespace(val.substr(exp.strlen))) {
                // Perform garbage collection.
                free_all_in_list(vals);
                exp.reset();

                // Fail
                return NULL;
            }

            // Trim spaces off of the name variable
            for (i = 0; nam[i] == ' ' || nam[i] == '\n' || nam[i] == '\t'; i++);
            nam = nam.substr(i);
            
            // Get the name of the new variable.
            string id = extract_identifier(nam);
            nam = nam.substr(id.length());

            if (!is_all_whitespace(nam)) {
                i = starts_with(nam, "(");
                if (i == -1) {
                    free_all_in_list(vals);
                    return NULL;
                }
                nam = nam.substr(i-1);
                
                // Assure closure
                i = index_of_closure(nam, '(', ')');
                if (i == -1 || !is_all_whitespace(nam.substr(i+1))) {
                    free_all_in_list(vals);
                    return NULL;
                }
                
                // Extract the argument list
                string seg = nam.substr(1, i-1);

                // Parse the arguments
                list<string> argv;
                if (!is_all_whitespace(seg)) while (true) {
                    // Push up to the front
                    int i;
                    for (i = 0; seg[i] == ' ' || seg[i] == '\n' || seg[i] == '\t'; i++);
                    seg = seg.substr(i);

                    string id = extract_identifier(seg);
                    if (id == "") {
                        // The content is not an argument.
                        free_all_in_list(vals);
                        return NULL;
                    } else {
                        // Apply the argument
                        argv.push_back(id);
                        seg = seg.substr(id.length());
                    }

                    if ((i = starts_with(seg, ",")) > 0)
                        // Another argument
                        seg = seg.substr(i);
                    else if (is_all_whitespace(seg))
                        // End of the list
                        break;
                    else {
                        // There is noise
                        free_all_in_list(vals);
                        return NULL;
                    }
                }

                // Store the elements into a list
                string *args = store_in_list<string>(argv, "");

                ids.push_back(id);
                vals.push_back(new LambdaExp(args, exp.value));
                recs.push_back(true);
            } else {
                // Push everything as is
                ids.push_back(id);
                vals.push_back(exp.value);
                recs.push_back(false);
            }

        }
        
        // Progress to the next expression.
        str = future.front();
        future.pop_front();
        
        // Identify the rest of the expression
        auto body = parse_sequence(str, future);
        if (!body) {
            // The future cannot be computed.
            free_all_in_list(vals);

            // Fail
            return NULL;
            
        }
        
        // Now, we can build a let-exp to represent our outcome
        string *vs = store_in_list<string>(ids, "");
        Exp *xs = store_in_list<Exp>(vals, NULL);
        bool *rs = new bool[ids.size()];
        for (i = ids.size()-1; i >= 0; i--) {
            rs[i] = recs.back();
            recs.pop_back();
        }
        
        // Thus, we finalize our expression.
        return new LetExp(vs, xs, body, rs);

    } else if ((i = starts_with(str, "from")) > 0) {
        if (future.size() == 0) {
            throw_err("parser", "import statement must be followed by a valid sequence:\n\t" + str.substr(0, 16));
            return NULL;
        }

        str = str.substr(i);

        // Extract the module name
        string module = extract_identifier(str);
        if (module == "") {
            if (str.find('\n') > 0)
                str = str.substr(0, str.find('\n'));
            throw_err("parser", "import expects identifier for module, thus the following name is invalid:\n\t" + str.substr(0, 32));
            return NULL;
        }
        str = str.substr(module.length());
        
        // Parse the import keyword
        if ((i = starts_with(str, "import")) < 0) {
            if (str.find('\n') >= 0)
                str = str.substr(0, str.find('\n'));
            throw_err("parser", "import-from statement missing import keyword; see:\n\t" + str.substr(0, 16));
            return NULL;
        }

        str = str.substr(i);

        list<string> ids = extract_statements(str, ',', false);

        for (i = 0; i < ids.size(); i++) {
            // Get the next value to check
            auto id = ids.front();
            ids.pop_front();
            
            // Reduce the string from the front
            int j;
            for (j = 0; id[j] == ' ' || id[j] == '\n' || id[j] == '\t'; j++);
            id = id.substr(j);
            
            // Reduce the string from the back
            for (j = id.length() - 1; j >= 0 && (id[j] == ' ' || id[j] == '\n' || id[j] == '\t'); j--);
            if (j < 0) {
                if (str.find('\n') >= 0)
                    str = str.substr(0, str.find('\n'));
                throw_err("parser", "import-from statement expects proper list of identifiers; see:\n\t" + str.substr(0, 16));
                return NULL;
            } else
                id = id.substr(0, j+1);
            
            if (is_identifier(id))
                // The string is legal
                ids.push_back(id);
            else {
                if (str.find('\n') >= 0)
                    str = str.substr(0, str.find('\n'));
                throw_err("parser", "import-from statement expects proper list of identifiers, but " + id + " is not an identifier; see:\n\t" + str.substr(0, 16));
                return NULL;
            }
        }
        
        // Proceed to the next step
        str = future.front();
        future.pop_front();
        auto body = parse_sequence(str, future);
        
        // Error check
        if (!body)
            return NULL;

        // Now, we build a final result.
        string *xs = store_in_list<string>(ids, "");

        Exp *ys = new Exp[ids.size()+1];

        ys[ids.size()] = NULL;
        for (int i = ids.size()-1; i >= 0; i--) {
            // Our expression will import the module and then grab the value.
            // I may come back to this and implement an expression specifically
            // for this functionality, as this is inefficient even with caching.
            ys[i] = new ImportExp(module, new DictAccessExp(new VarExp(module), ids.back()));

            // Clean the item out of the list.
            ids.pop_back();
        }

        return new LetExp(xs, ys, body);

    } else if ((i = starts_with(str, "import")) > 0) {
        // The import must be followed by something that uses the import
        if (future.size() == 0) {
            throw_err("parser", "import statement must be followed by a valid sequence:\n\t" + str.substr(0, 16));
            return NULL;
        }

        str = str.substr(i);

        list<string> modules;
        list<string> names;
        
        while (true) {
            string module = extract_identifier(str);
            if (module == "") {
                if (str.find('\n') > 0)
                    str = str.substr(0, str.find('\n'));
                throw_err("parser", "import expects identifier for module, thus the following name is invalid:\n\t" + str.substr(0, 32));
                return NULL;
            }
            str = str.substr(module.length());
            
            string name;
            if ((i = starts_with(str, "as")) > 0) {
                str = str.substr(i);
                name = extract_identifier(str);

                if (module == "") {
                    if (str.find('\n') > 0)
                        str = str.substr(0, str.find('\n'));
                    throw_err("parser", "import expects identifier for name, thus the following name is invalid:\n\t" + str.substr(0, 32));
                    return NULL;
                }

                str = str.substr(name.length());
            } else
                name = module;

            modules.push_back(module);
            names.push_back(name);
            
            if ((i = starts_with(str, ",")) > 0) {
                // Progress for the next iteration.
                str = str.substr(i);
            } else if (is_all_whitespace(str))
                // There are no more modules to import.
                break;
            else {
                // Module definition is followed by garbage.
                throw_err("parser", "import of module " + module + " as " + name + " is followed by extra symbols:\n\t" + str);
                return NULL;
            }
        }
        
        str = future.front();
        future.pop_front();
        auto body = parse_sequence(str, future);

        if (!body)
            return NULL;

        while (names.size()) {
            // Apply another import
            body = new ImportExp(modules.back(), names.back(), body);
            // Throw away the old values
            modules.pop_back();
            names.pop_back();
        }

        return body;

    } else if ((i = starts_with(str, "type")) > 0) {
        if (future.size() == 0) {
            if (str.find('\n') >= 0)
                str = str.substr(0, str.find('\n'));

            throw_err("parser", "adt definition must be followed by a valid sequence:\n\t" + str.substr(0, 16));

            return NULL;
        }

        // Defining a new ADT
        str = str.substr(i);
        
        // Extract the name of the type
        string name = extract_identifier(str);
        if (name == "") return NULL;
        else str = str.substr(name.length());
        
        // Extract the equals sign
        if ((i = starts_with(str, "=")) == -1)
            return NULL;
        else
            str = str.substr(i);
        
        // Extract a feature set
        list<string> states = extract_statements(str, '|', false);
        
        list<pair<string, list<Type*>>> defs;
        
        while (states.size()) {
            // Extract the type statement
            string state = states.front();

            for (i = 0; state[i] == ' ' || state[i] == '\n' || state[i] == '\t'; i++);
            state = state.substr(i);
            
            string id = extract_identifier(state);
            if (id == "")
                break;
            else
                state = state.substr(id.length());
            
            if ((i = starts_with(state, "(")) == -1)
                break;
            
            // Find the opening of the arguments
            i = index_of_char(state, '(');
            if (i == -1) break;

            state = state.substr(i);
            
            // Find closure.
            int j = index_of_closure(state, '(', ')');
            if (j == -1 || !is_all_whitespace(state.substr(j+1))) break;
            else state = state.substr(i+1, j-1);

            // Create a space for a definition
            pair<string, list<Type*>> def;
            def.first = id;
            
            if (!is_all_whitespace(state)) {
                // Parse for arguments.
                list<string> lsts = extract_statements(state, ',', false);
                
                // Go through each of the definitions
                while (lsts.size()) {
                    // Get the string identifying the type
                    string t = lsts.front();
                    
                    // Parse the type, if possible.
                    result<Type> type = parse_type(t);
                    if (!type.value || !is_all_whitespace(t.substr(type.strlen))) {
                        delete type.value;
                        break;
                    } else
                        def.second.push_back(type.value);

                    // Mark the item as properly handled.
                    lsts.pop_front();
                }
                
                // Apply the list
                defs.push_back(def);
                
                // If any definitions remain, something went wrong.
                if (lsts.size())
                    break;

            } else
                // We only care to add the trivial type
                defs.push_back(def);
            
            // Mark the statement as successfully done.
            states.pop_front();
        }

        // Perform GC
        if (states.size()) {
            // For each item for each argument, perform GC
            for (auto jt = defs.begin(); jt != defs.end(); jt++)
                free_all_in_list<Type*>(jt->second);

            return NULL;
        }

        // Proceed to the next step
        str = future.front();
        future.pop_front();
        auto body = parse_sequence(str, future);
        
        // Error check
        if (!body) {
            // For each item for each argument, perform GC
            for (auto jt = defs.begin(); jt != defs.end(); jt++)
                free_all_in_list<Type*>(jt->second);

            return NULL;
        }

        list<string> ids;
        list<Type**> argss;

        // Now, we can correctly evaluate.
        for (auto it = defs.begin(); it != defs.end(); it++) {
            ids.push_back(it->first);
            argss.push_back(store_in_list<Type*>(it->second, NULL));
        }
        
        // Build the end result.
        return new AdtDeclarationExp(name,
            store_in_list<string>(ids, ""),
            store_in_list<Type**>(argss, NULL),
            body);
    }
    
    // We may have a sequence. Parse the first section.
    Exp E = parse_statement(str);
    if (!E) return NULL;
    
    // Now, handle the rest
    if (future.size() == 0) {
        // Nothing else follows. Hence, E is all we need.
        return E;
    }
    
    str = future.front();
    future.pop_front();

    Exp F = parse_sequence(str, future);
    if (!F) {
        // We couldn't generate the rest of the program.
        delete E;
        return NULL;
    } else if (isExp<SequenceExp>(F)) {
        // Add to the sequence.
        ((SequenceExp*) F)->getSeq()->add(0, E);

        // Thus, our modified sequence is our result<Expression>.
        return F;
    } else {
        // We simply create a new sequence to reflect the outcome.
        auto seq = new LinkedList<Exp>;
        seq->add(0, F);
        seq->add(0, E);
        return new SequenceExp(seq);
    }
    

}

Exp parse_program(string str) {
    auto statements = extract_statements(str);
    
    // If statements cannot be extracted, we cannot build an expression.
    if (statements.size() == 0)
        return NULL;

    string first = statements.front();
    statements.pop_front();
    
    return parse_sequence(first, statements);

}

