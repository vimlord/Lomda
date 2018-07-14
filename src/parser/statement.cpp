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

