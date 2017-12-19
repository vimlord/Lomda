#include "bnf.hpp"
#include "expression.hpp"

using namespace std;

/**
 * <for-exp> ::= 'for' <id> 'in' <pemdas> <codeblk>
 */
ParsedPrgms parseForExp(string str, bool ends) {
    
    int len, i;
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    len = parseLit(str, "for");
    if (len < 0) return res;
    str = str.substr(len);
    
    // The variable name to extend with
    parsed_id id = parseId(str);
    if (id.len < 0) return res;
    len += id.len;
    str = str.substr(id.len);

    // in keyword
    if ((i = parseLit(str, "in")) < 0) return res;
    str = str.substr(i);
    len += i;

    // Now, parse for the list to iterate over
    ParsedPrgms lists = parseCodeBlock(str, false);
    
    while (!lists->isEmpty()) {
        parsed_prgm list = lists->remove(0);

        string s = str.substr(list.len);
        int length = len + list.len;

        // The possible false bodies need to be found
        ParsedPrgms bodies = parseCodeBlock(s, ends);
        while (!bodies->isEmpty()) {
            parsed_prgm body = bodies->remove(0);
            
            // Now, we can generate an outcome
            body.len += length;
            body.item = new ForExp(id.item, list.item, body.item);
            res->add(0, body);

            list.item = list.item->clone();

        }
        delete bodies;
    }
    delete lists;

    return res;
}

/**
 * <if-exp> ::= 'if' <pemdas> 'then' <codeblk> 'else' <codeblk>
 */
ParsedPrgms parseIfExp(string str, bool ends) {
    
    int len, i;
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    len = parseLit(str, "if");
    if (len < 0) return res;
    str = str.substr(len);
    
    // The possible conditionals
    ParsedPrgms conditions = parsePemdas(str, false);
    
    while (!conditions->isEmpty()) {
        parsed_prgm cond = conditions->remove(0);

        string s = str.substr(cond.len);
        int length = len + cond.len;;
        
        if ((i = parseLit(s, "then")) < 0) {
            delete cond.item;
            continue;
        }

        s = s.substr(i);
        length += i;

        // Now, parse for the truth body
        ParsedPrgms tBodies = parseCodeBlock(s, false);
        
        while (!tBodies->isEmpty()) {
            parsed_prgm tbody = tBodies->remove(0);

            string st = s.substr(tbody.len);
            int l = length + tbody.len;

            // Next, we will filter for the 'else' keyword
            if ((i = parseLit(st, "else")) < 0) {
                // Remove the candidate
                delete tbody.item;
                continue;
            } else {
                l += i;
                st = st.substr(i);
            }
            
            // The possible false bodies need to be found
            ParsedPrgms fBodies = parseCodeBlock(st, ends);
            while (!fBodies->isEmpty()) {
                parsed_prgm fbody = fBodies->remove(0);
                
                // Now, we can generate an outcome
                fbody.len += l;
                fbody.item = new IfExp(cond.item, tbody.item, fbody.item);
                res->add(0, fbody);

                cond.item = cond.item->clone();
                tbody.item = tbody.item->clone();
            }
            delete fBodies;
        }
        delete tBodies;
    }
    delete conditions;

    return res;
}

/**
 * <insert-exp> ::= 'insert' <pemdas> 'into' <pemdas> 'at' <pemdas>
 */
ParsedPrgms parseInsertExp(string str, bool ends) {
    
    int len, i;
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    len = parseLit(str, "insert");
    if (len < 0) return res;
    str = str.substr(len);
    
    // The possible elements
    ParsedPrgms elements = parsePemdas(str, false);
    
    while (!elements->isEmpty()) {
        parsed_prgm elem = elements->remove(0);

        string s = str.substr(elem.len);
        int length = len + elem.len;;
        
        if ((i = parseLit(s, "into")) < 0) {
            delete elem.item;
            continue;
        }

        s = s.substr(i);
        length += i;

        // Now, parse for the truth body
        ParsedPrgms lists = parsePemdas(s, false);
        
        while (!lists->isEmpty()) {
            parsed_prgm lst = lists->remove(0);

            string st = s.substr(lst.len);
            int l = length + lst.len;

            // Next, we will filter for the 'else' keyword
            if ((i = parseLit(st, "at")) < 0) {
                // Remove the candidate
                delete lst.item;
                continue;
            } else {
                l += i;
                st = st.substr(i);
            }
            
            // The possible false bodies need to be found
            ParsedPrgms indices = parsePemdas(st, ends);
            while (!indices->isEmpty()) {
                parsed_prgm idx = indices->remove(0);
                
                // Now, we can generate an outcome
                idx.len += l;
                idx.item = new ListAddExp(lst.item, idx.item, elem.item);
                res->add(0, idx);

                lst.item = lst.item->clone();
                elem.item = elem.item->clone();

            }
            delete indices;
        }
        delete lists;
    }
    delete elements;

    return res;
}

/**
 * <let-exp> ::= 'let' (<id> ('=' | 'equal') <pemdas>)+ ';' <program>
 */
ParsedPrgms parseLetExp(string str, bool ends) {
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    int i;
    
    i = parseLit(str, "let");
    if (i < 0) {
        // This is not a let expression
        return res;
    }

    struct arglist lst;
    lst.len = i;
    lst.list = new LinkedList<struct arg>;
    
    // The list of arglists
    LinkedList<struct arglist> options;
    options.add(0, lst);

    // The list of complete arglists
    LinkedList<struct arglist> lists;
    
    // Parse for arguments
    while (!options.isEmpty()) {
        // Get an argument list option
        lst = options.remove(0);
        string s = str.substr(lst.len);

        int len = 0;
        
        // Parse the identifier
        parsed_id d_id = parseId(s);
        if (d_id.len < 0) {
            while (!lst.list->isEmpty()) delete lst.list->remove(0).exp;
            delete lst.list;
            continue;
        } else {
            s = s.substr(d_id.len);
            len += d_id.len;
        }
        
        // Parse literal
        if ((i = parseLit(s, "equal")) < 0 &&
            (i = parseLit(s, "=")) < 0) {
            while (!lst.list->isEmpty()) delete lst.list->remove(0).exp;
            delete lst.list;
            continue;
        } else {
            s = s.substr(i);
            len += i;
        }
        
        //std::cout << "Searching for let-exp args in '" << s << "'\n";

        // Parse all possible expressions
        ParsedPrgms args = parsePemdas(s, false);

        //std::cout << "found " << args->size() << " arg candidates\n";

        if (args->isEmpty()) {
            // No possible way to parse arguments
            delete args;
            delete lst.list;
            continue;
        }

        // Act on each possible branch
        while (!args->isEmpty()) {
            parsed_prgm prgm = args->remove(0);

            struct arglist newlst;
            newlst.len = lst.len + len + prgm.len;

            // Rebuild the list
            newlst.list = new LinkedList<struct arg>;
            
            // Add each item from the old list
            Iterator<int, struct arg> *lstit = lst.list->iterator();
            i = 0;
            while (lstit->hasNext()) newlst.list->add(i++, lstit->next());
            delete lstit;

            // Append the newest expression
            struct arg arg;
            arg.id = d_id.item;
            arg.exp = prgm.item;
            newlst.list->add(i, arg);

            // Next, we want to split into three categories:
            // (0) : Semicolon immediately follows: end the let
            // (1) : Comma immediately follows: there is more to parse
            // (2) : Otherwise, the tree is bad
            string st = s.substr(prgm.len);
            int semi = parseLit(st, ";"); // Perhaps we reached a semicolon terminator?
            int comm = parseLit(st, ","); // Maybe we reached a comma separator?

            if (semi < 0 && comm < 0) { // If not, the usage is illegal
                delete newlst.list;
                continue;
            } else {
                // Adjust the length
                newlst.len += semi > comm ? semi : comm;

                //std::cout << "arg list defines " << arg.id << " as " << *(arg.exp) << "\n";

                // Now, we add this argument list as an option. Adding to the
                // front results in DFS, placing at end is approximately BFS.
                if (semi > comm) {
                    //std::cout << "completed an argument list\n";
                    lists.add(0, newlst);
                } else {
                    //std::cout << "argument list is incomplete\n";
                    options.add(0, newlst);
                }
            }
        }

        delete args;
        delete lst.list;

    }

    //std::cout << "let-exp candidates: " << lists.size() << "\n";
    
    while (!lists.isEmpty()) {
        lst = lists.remove(0);
        string s = str.substr(lst.len);
        //std::cout << "must evaluate expression for '" << s << "'\n";

        // Calculate all possible branches
        ParsedPrgms bodies = parseProgram(s, ends);

        //std::cout << "let-body candidates: " << bodies->size() << "\n";

        while (!bodies->isEmpty()) {
            // Here, we will modify a struct to make the changes.
            parsed_prgm prgm = bodies->remove(0);

            // Add to the list of possible expressions
            string *ids = new string[lst.list->size() + 1];
            Expression **vals = new Expression*[lst.list->size() + 1];
            
            auto lit = lst.list->iterator();
            for (i = 0; lit->hasNext(); i++) {
                struct arg a = lit->next();
                ids[i] = a.id;
                vals[i] = a.exp->clone();
            }
            ids[i] = ""; vals[i] = NULL;
            delete lit;

            prgm.len += lst.len;
            prgm.item = new LetExp(ids, vals, prgm.item); // Collapse into target
            //std::cout << "Found let-exp '" << *(prgm.item) << "'\n";
            
            // Add the result
            res->add(0, prgm);
        }

        // Garbage collection
        delete bodies;

        while (!lst.list->isEmpty()) {
            Expression *v = lst.list->remove(0).exp;
            delete v;
        }
        delete lst.list;
    }

    return res;
}

/**
 * top-level pemdas-exp
 */
ParsedPrgms parsePemdas(string str, bool ends) {
    ParsedPrgms res = parseSetExp(str, ends);

    // Parse for lambdas
    ParsedPrgms tmp = parseLambdaExp(str, ends);
    while (!tmp->isEmpty()) res->add(0, tmp->remove(0));
    delete tmp;

    // Parse for derivative
    tmp = parseDerivative(str, ends);
    while (!tmp->isEmpty()) res->add(0, tmp->remove(0));
    delete tmp;


    return res;
}

/**
 * <remove-exp> ::= 'remove' <pemdas> 'from' <pemdas>
 */
ParsedPrgms parseRemoveExp(string str, bool ends) {
    
    int len, i;
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    len = parseLit(str, "remove");
    if (len < 0) return res;
    str = str.substr(len);
    
    // The possible indices
    ParsedPrgms indices = parsePemdas(str, false);
    
    while (!indices->isEmpty()) {
        parsed_prgm idx = indices->remove(0);

        string s = str.substr(idx.len);
        int length = len + idx.len;
        
        if ((i = parseLit(s, "from")) < 0) {
            delete idx.item;
            continue;
        }

        s = s.substr(i);
        length += i;

        // Now, parse for the truth body
        ParsedPrgms lists = parsePemdas(s, ends);
        
        while (!lists->isEmpty()) {
            parsed_prgm lst = lists->remove(0);
            lst.len += length;
            
            lst.item = new ListRemExp(lst.item, idx.item);
            res->add(0, lst);

            idx.item = idx.item->clone();
        }
        delete lists;
    }
    delete indices;

    return res;
}

/**
 * <while-exp> ::= 'while' <pemdas> <codeblk>
 */
ParsedPrgms parseWhileExp(string str, bool ends) {
    int len, i;
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    len = parseLit(str, "while");
    if (len < 0) return res;
    str = str.substr(len);

    ParsedPrgms conditions = parsePemdas(str, false);
    while (!conditions->isEmpty()) {
        parsed_prgm cond = conditions->remove(0);

        string s = str.substr(cond.len);
        int length = len + cond.len;;
        
        // Now, parse for the body
        ParsedPrgms bodies = parseCodeBlock(s, ends);

        // Then, we apply each of the bodies to the outcome
        while (!bodies->isEmpty()) {
            parsed_prgm body = bodies->remove(0);
            string st = s.substr(body.len);
            
            body.item = new WhileExp(cond.item, body.item);
            
            res->add(0, body);
        }

    }

    delete conditions;
    return res;
}

