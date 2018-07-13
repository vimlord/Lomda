#include "parser.hpp"

using namespace std;

Exp parse_sequence(string str, list<string> future) {
    int i;

    if ((i = starts_with(str, "let")) > 0) {
        // Let expression
        if (future.empty()) {
            return NULL;
        }

        // Parse the argument list
        auto args = extract_statements(str.substr(i), ',', false);
        if (args.empty()) {
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
        if (future.empty()) {
            return NULL;
        }

        str = str.substr(i);

        // Extract the module name
        string module = extract_identifier(str);
        if (module == "") {
            return NULL;
        }
        str = str.substr(module.length());
        
        // Parse the import keyword
        if ((i = starts_with(str, "import")) < 0) {
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
                return NULL;
            } else
                id = id.substr(0, j+1);
            
            if (is_identifier(id))
                // The string is legal
                ids.push_back(id);
            else {
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
        if (future.empty()) {
            return NULL;
        }

        str = str.substr(i);

        list<string> modules;
        list<string> names;
        
        while (true) {
            string module = extract_identifier(str);
            if (module == "") {
                return NULL;
            }
            str = str.substr(module.length());
            
            string name;
            if ((i = starts_with(str, "as")) > 0) {
                str = str.substr(i);
                name = extract_identifier(str);

                if (module == "") {
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
        if (future.empty()) {
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
    if (future.empty()) {
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


