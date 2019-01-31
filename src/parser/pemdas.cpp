#include "parser.hpp"

#include <vector>

using namespace std;

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
    } else if ((i = starts_with(str, "fold")) > 0) {
            result<Expression> lst = parse_pemdas(str.substr(i));
            if (!lst.value) {
                return base;
            } else
                i += lst.strlen;

            int j;
            
            if ((j = starts_with(str.substr(i), "into")) < 0) {
                delete lst.value;
                return base;
            } else
                i += j;
            
            result<Expression> func = parse_body(str.substr(i));
            if (!func.value) {
                delete lst.value;
                return base;
            } else
                i += func.strlen;

            if ((j = starts_with(str.substr(i), "from")) < 0) {
                delete lst.value;
                delete func.value;
                return base;
            } else
                i += j;

            result<Expression> init = parse_body(str.substr(i), true);
            if (!init.value) {
                delete lst.value;
                delete func.value;
                return base;
            } else
                i += init.strlen;

            // Build the end result
            base.value =  new FoldExp(lst.value, func.value, init.value);
            base.strlen = i;

        } else if ((i = starts_with(str, "map")) > 0) {
            // Parse the conditional
            result<Expression> func = parse_pemdas(str.substr(i));
            if (!func.value)
                return base;
            else
                i += func.strlen;

            int j;
            
            // Then keyword
            if ((j = starts_with(str.substr(i), "over")) < 0) {
                delete func.value;
                return base;
            } else
                i += j;
            
            // Parse the first body
            result<Expression> lst = parse_body(str.substr(i));
            if (!lst.value) {
                delete func.value;
                return base;
            } else
                i += lst.strlen;

            // Build the end result
            base.value =  new MapExp(func.value, lst.value);
            base.strlen = i;

        } else if ((i = starts_with(str, "left")) > 0) {
            // Parse for the 'of' keyword
            int j = starts_with(str.substr(i), "of");
            if (j <= 0) {
                base.reset();
                return base;
            }

            // Build the result if possible
            base = parse_pemdas(str.substr(i+j), 2);
            if (base.value) {
                base.value = new TupleAccessExp(base.value, false);
                base.strlen += i + j;
            } else
                return base;

        } else if ((i = starts_with(str, "right")) > 0) {
            // Parse for the 'of' keyword
            int j = starts_with(str.substr(i), "of");
            if (j <= 0) {
                base.reset();
                return base;
            }
            
            // Build the result if possible
            base = parse_pemdas(str.substr(i+j), 2);
            if (base.value) {
                base.value = new TupleAccessExp(base.value, true);
                base.strlen += i + j;
            }
        } else if ((i = starts_with(str, "switch")) > 0) {
            // Extract an ADT
            result<Expression> adt = parse_pemdas(str.substr(i), 1); // Identification is limited to low level definitions
            if (!adt.value) return base;
            else i += adt.strlen;

            int j;

            if ((j = starts_with(str.substr(i), "in")) == -1) {
                delete adt.value;
                return base;
            } else
                i += j;
            
            // Extract a statement list
            list<string> states = extract_statements(str.substr(i), '|', false);
            
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
                
                // Parse the body. It only needs to terminate inside,
                // as the delimiter '|' completely encapsulates.
                result<Expression> body = parse_body(state, states.size() > 1);
                if (!body.value) break;
                else bodies.push_back(body.value);

                // Mark as complete
                states.pop_front();
                
                // If we reached the end, then we can set the true length of the expression
                i = str.length() - (state.length() - body.strlen);
            }

            if (!states.empty()) {
                // The evaluation failed
                delete adt.value;
                while (bodies.size()) { delete bodies.front(); bodies.pop_front(); }
                while (idss.size()) { delete[] idss.front(); idss.pop_front(); }
                return base;
            }

            base.value = new SwitchExp(
                    adt.value,
                    store_in_list<string>(names, ""),
                    store_in_list<string*>(idss, NULL),
                    store_in_list<Exp>(bodies, NULL)
            );
            base.strlen = i;
        } else if ((i = starts_with(str, "d/d")) > 0) {
            // Attempt to extract the variable
            string x = extract_identifier(str.substr(index_of_char(str, 'd') + 3));

            if (x == "") {
                base.reset();
                return base;
            } else {
                i += x.length();
            }
            
            // Evaluate for a derivative
            base = parse_pemdas(str.substr(i), order);
            if (base.value) {
                base.value = new DerivativeExp(base.value, x);
                base.strlen += i;
            }
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
            auto list = new ListExp;
            base.value = list;

            int i = 0;
            for (auto it = argv.begin(); it != argv.end(); it++, i++) {
                // Attempt to extract the item
                string x = *it;
                result<Expression> arg = parse_pemdas(x);
                if (!arg.value || !is_all_whitespace(x.substr(arg.strlen))) {
                    base.reset();
                    return base;
                } else {
                    list->add(i, arg.value);
                }
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
            for (unsigned int i = 0; i < literal.length(); i++) {
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
        } else if ((len = starts_with(str, "|")) > 0) {
            // Magnitude
            len = index_of_char(str, '|');

            int j = index_of_closure(str.substr(len), '|', '|');
            if (j == -1) {
                base.reset();
                return base;
            }

            string seg = str.substr(len+1, j-1);

            bool norm = false;
            if (seg[0] == '|' && seg[seg.length()-1] == '|') {
                seg = seg.substr(1, seg.length()-2);
                norm = true;
            }
            
            // Extract a statement from the contents.
            base = parse_pemdas(seg);

            if (base.strlen == -1) {
                base.reset();
                return base;
            }
            
            base.value = norm 
                    ? (Exp) new NormExp(base.value) 
                    : (Exp) new MagnitudeExp(base.value);

            base.strlen = len + j + 1;
            str = str.substr(base.strlen);
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
                } else if (var == "asin" || var == "arcsin") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::ASIN);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "acos" || var == "arccos") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::ACOS);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "atan" || var == "arctan") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::ATAN);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "sinh") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::SINH);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "cosh") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::COSH);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "tanh") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::TANH);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "asinh" || var == "arcsinh") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::ASINH);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "acosh" || var == "arccosh") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::ACOSH);
                    if (y.value) {
                        str = str.substr(y.strlen);
                        y.strlen += base.strlen;
                        base = y;
                    }
                } else if (var == "atanh" || var == "arctanh") {
                    auto y = parse_stdmathfn(str, StdMathExp::MathFn::ATANH);
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
                    str = str.substr(body.strlen);

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
                int Z = 0;
                float R = 0;

                // Attempt to extract a real
                try { R = stof(str, &Rlen); }
                catch (std::out_of_range &oor) { Rlen = -1; }
                catch (std::invalid_argument &ia) { Rlen = -1; }

                // Attempt to extract an integer
                try { Z = stoi(str, &Zlen, 10); }
                catch (std::out_of_range &oor) { Zlen = -1; }
                catch (std::invalid_argument &ia) { Zlen = -1; }

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

        if ((i = starts_with_any(str, {"==", "equals"})) > 0)
            op = EQ;
        else if ((i = starts_with(str, "!=")) > 0)
            op = NEQ;
        else if ((i = starts_with(str, "is")) > 0) {
            int j;
            if ((j = starts_with(str.substr(i), "not")) > 0) {
                op = NEQ;
                i += j;
            } else {
                op = EQ;
            }
        }
        
        if (i > 0) {
            str = str.substr(i);
            result<Expression> alt = parse_pemdas(str, 8);
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
            int i;
            if ((i = starts_with(str, "and")) == -1)
                break;

            str = str.substr(i);

            result<Expression> next = parse_pemdas(str, 9);
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
            int i;
            if ((i = starts_with(str, "or")) == -1)
                break;

            str = str.substr(i);

            result<Expression> next = parse_pemdas(str, 10);
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
        int i;
        if ((i = starts_with(str, "if")) > 0) {
            result<Expression> cond = parse_pemdas(str.substr(i));
            if (!cond.value) {
                base.reset();
                return base;
            }
            
            base.strlen += i + cond.strlen;
            str = str.substr(i + cond.strlen);

            if ((i = starts_with(str, "else")) == -1) {
                cond.reset();
                base.reset();
                return base;
            } else {
                base.strlen += i;
                str = str.substr(i);
            }

            result<Expression> other = parse_pemdas(str);
            if (!other.value) {
                cond.reset();
                base.reset();
                return base;
            } else {
                base.strlen += other.strlen;
                str = str.substr(other.strlen);
            }

            base.value = new IfExp(cond.value, base.value, other.value);
        }
    }
    
    if (order >= 12) {
        // Assignment
        int i;
        if ((i = starts_with(str, "=")) > 0) {
            result<Expression> next = parse_pemdas(str.substr(i), order);
            if (!next.value) {
                base.reset();
                return base;
            }

            base.value = new SetExp(base.value, next.value);
            base.strlen += i + next.strlen;
            
            str = str.substr(i + next.strlen);
        }
    }

    if (order >= 13) {
        int i;
        if ((i = starts_with(str, ",")) > 0) {
            result<Expression> next = parse_pemdas(str.substr(i), order);
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


