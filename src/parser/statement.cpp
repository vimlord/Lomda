#include "parser.hpp"

using namespace std;


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
            cond.reset();
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
    } else if ((i = starts_with(str, "insert")) > 0) {
        str = str.substr(i);
        
        // Parse the valitional
        result<Expression> val = parse_pemdas(str);
        if (!val.value)
            return NULL;
        else
            str = str.substr(val.strlen);
        
        // Then keyword
        if ((i = starts_with(str, "into")) < 0) {
            val.reset();
            return NULL;
        } else
            str = str.substr(i);
        
        // Parse the first body
        result<Expression> lst = parse_body(str);
        if (!lst.value) {
            delete val.value;
            return NULL;
        } else
            str = str.substr(lst.strlen);

        // Else keyword
        if ((i = starts_with(str, "at")) < 0) {
            delete val.value;
            delete lst.value;
            return NULL;
        } else
            str = str.substr(i);

        // Parse the seval body
        result<Expression> idx = parse_body(str, true);
        if (!idx.value) {
            delete val.value;
            delete lst.value;
            return NULL;
        }

        // Build the end result
        return new ListAddExp(lst.value, idx.value, val.value);
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
        
    } else if ((i = starts_with(str, "remove")) > 0) {
        str = str.substr(i);
        
        // Then keyword
        if ((i = starts_with(str, "from")) < 0) {
            return NULL;
        } else
            str = str.substr(i);
        
        // Parse the first body
        result<Expression> lst = parse_body(str);
        if (!lst.value) {
            return NULL;
        } else
            str = str.substr(lst.strlen);

        // Else keyword
        if ((i = starts_with(str, "at")) < 0) {
            delete lst.value;
            return NULL;
        } else
            str = str.substr(i);

        // Parse the seval body
        result<Expression> idx = parse_body(str, true);
        if (!idx.value) {
            delete lst.value;
            return NULL;
        }

        // Build the end result
        return new ListRemExp(lst.value, idx.value);
    } else if ((i = starts_with(str, "switch")) > 0) {
        str = str.substr(i);
        
        // Extract an ADT
        result<Expression> adt = parse_pemdas(str, 1); // Identification is limited to low level definitions
        if (!adt.value) return NULL;
        else str = str.substr(adt.strlen);

        if ((i = starts_with(str, "in")) == -1) {
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

