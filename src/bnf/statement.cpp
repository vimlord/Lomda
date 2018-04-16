#include "bnf.hpp"
#include "expression.hpp"

using namespace std;

/**
 * <fold-exp> ::= 'fold' <pemdas> 'into' <pemdas> 'at' <pemdas>
 */
ParsedPrgms parseFoldExp(string str, bool ends) {
    
    int len, i;
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    len = parseLit(str, "fold");
    if (len < 0) return res;
    str = str.substr(len);

    // The possible lists
    ParsedPrgms lists = parsePemdas(str, false);
    
    while (!lists->isEmpty()) {
        parsed_prgm lst = lists->remove(0);

        string s = str.substr(lst.len);
        int length = len + lst.len;;
        
        if ((i = parseLit(s, "into")) < 0) {
            delete lst.item;
            continue;
        }

        s = s.substr(i);
        length += i;

        // Now, parse for the truth body
        ParsedPrgms funcs = parsePemdas(s, false);
        
        while (!funcs->isEmpty()) {
            parsed_prgm fn = funcs->remove(0);

            string st = s.substr(fn.len);
            int l = length + fn.len;

            // Next, we will filter for the 'else' keyword
            if ((i = parseLit(st, "from")) < 0) {
                // Remove the candidate
                delete fn.item;
                continue;
            } else {
                l += i;
                st = st.substr(i);
            }
            
            // The possible false bodies need to be found
            ParsedPrgms bases = parsePemdas(st, ends);
            while (!bases->isEmpty()) {
                parsed_prgm base = bases->remove(0);
                
                // Now, we can generate an outcome
                base.len += l;
                base.item = new FoldExp(lst.item, fn.item, base.item);
                res->add(0, base);

                fn.item = fn.item->clone();
                lst.item = lst.item->clone();

            }
            delete bases;
        }
        delete funcs;
    }
    delete lists;

    return res;
}

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

ParsedPrgms parseImportExp(string str, bool ends) {
    ParsedPrgms res = new LinkedList<parsed_prgm>;
    int i, len;

    if ((i = parseLit(str, "import")) < 0) return res;
    
    str = str.substr(i);
    len = i;

    parsed_id module = parseId(str);
    if (module.len < 0) return res;

    len += module.len;
    str = str.substr(module.len);
    
    // There are three possibilities:
    // 1) The module is renamed
    // 2) The module is not renamed, and is followed by code
    // 3) The import is all that remains
    
    string name;
    if ((i = parseLit(str, "as")) >= 0) {
        len += i;
        str = str.substr(i);

        parsed_id name = parseId(str);
        if (name.len < 0) return res;
        
        len += name.len;
        str = str.substr(name.len);
    } else
        name = module.item;

    if ((i = parseLit(str, ";")) >= 0) {
        len += i;
        str = str.substr(i);

        // Now, we find every possible branch
        ParsedPrgms progs = parseProgram(str, ends);

        while (!progs->isEmpty()) {
            parsed_prgm p = progs->remove(0);
            
            p.item = new ImportExp(module.item, name, p.item);
            p.len += len;

            res->add(0, p);
        }

    } else if (!ends || parseSpaces(str) == str.length()) {
        parsed_prgm p;
        p.item = new ImportExp(module.item, name, NULL);
        p.len = len;
        res->add(0, p);
    }

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
        if ((i = parseLit(s, "equal")) >= 0 ||
            (i = parseLit(s, "=")) >= 0) {
            // Progress the string
            s = s.substr(i);
            len += i;

            // Parse all possible expressions
            ParsedPrgms args = parsePemdas(s, false);

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
                auto lstit = lst.list->iterator();
                i = 0;
                while (lstit->hasNext()) newlst.list->add(i++, lstit->next());
                delete lstit;

                // Append the newest expression
                struct arg arg;
                arg.id = d_id.item;
                arg.exp = prgm.item;
                arg.rec = false;
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

        } else if ((i = parseLit(s, "(")) >= 0) {
            // We are parsing for a function of some kind.
            s = s.substr(i);
            len += i;
            
            // The argument list
            LinkedList<string> xs;

            bool parsed = true;

            if ((i = parseLit(s, ")")) >= 0) {
                s = s.substr(i);
                len += i;
                
                // Check for equal sign
                if ((i = parseLit(s, "=")) >= 0) {
                    s = s.substr(i);
                    len += i;
                } else
                    parsed = false;

            } else while (1) {
                 
                parsed_id arg = parseId(s);
                if (arg.len < 0)
                    // The argument set is invalid
                    return res;
                else
                    xs.add(xs.size(), arg.item);
                
                // Progress the string
                s = s.substr(arg.len);
                len += arg.len;

                if ((i = parseLit(s, ")")) >= 0) {
                    // End of arguments
                    s = s.substr(i);
                    len += i;

                    if ((i = parseLit(s, "=")) >= 0) {
                        s = s.substr(i);
                        len += i;
                    } else
                        parsed = false;

                    break;
                } else if ((i = parseLit(s, ",")) >= 0) {
                    s = s.substr(i);
                    len += i;
                } else {
                    // Something is in place of the comma
                    parsed = false;
                    break;
                }
            }

            if (!parsed) {
                // The argument list is bad
                while (!lst.list->isEmpty()) delete lst.list->remove(0).exp;
                delete lst.list;
                continue;
            }

           //std::cout << "parse for body in " << s << "\n";

            ParsedPrgms args = parseCodeBlock(s, false);

            // Act on each possible branch
            while (!args->isEmpty()) {
                parsed_prgm prgm = args->remove(0);

               //std::cout << "found " << prgm.item->toString() << "\n";

                struct arglist newlst;
                newlst.len = lst.len + len + prgm.len;

                // Rebuild the list
                newlst.list = new LinkedList<struct arg>;
                
                // Add each item from the old list
                auto lstit = lst.list->iterator();
                i = 0;
                while (lstit->hasNext()) newlst.list->add(i++, lstit->next());
                delete lstit;

                // Build the lambda's argument list
                string *as = new string[xs.size()+1];
                auto it = xs.iterator();
                for (int i = 0; it->hasNext(); i++)
                    as[i] = it->next();

                // Append the newest expression
                struct arg arg;
                arg.id = d_id.item;
                arg.exp = new LambdaExp(as, prgm.item);
                arg.rec = true;
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


        } else {
            while (!lst.list->isEmpty()) delete lst.list->remove(0).exp;
            delete lst.list;
        }

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
            bool *rec = new bool[lst.list->size()];
            
            auto lit = lst.list->iterator();
            for (i = 0; lit->hasNext(); i++) {
                struct arg a = lit->next();
                ids[i] = a.id;
                vals[i] = a.exp->clone();
                rec[i] = a.rec;
            }
            ids[i] = ""; vals[i] = NULL;
            delete lit;

            prgm.len += lst.len;
            prgm.item = new LetExp(ids, vals, prgm.item, rec); // Collapse into target
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
 * <map-exp> ::= 'map' <pemdas> 'to' <pemdas>
 */
ParsedPrgms parseMapExp(string str, bool ends) {
    
    int len, i;
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    len = parseLit(str, "map");
    if (len < 0) return res;
    str = str.substr(len);
    
    // The possible indices
    ParsedPrgms functions = parsePemdas(str, false);
    
    while (!functions->isEmpty()) {
        parsed_prgm fun = functions->remove(0);

        string s = str.substr(fun.len);
        int length = len + fun.len;
        
        if ((i = parseLit(s, "over")) < 0) {
            delete fun.item;
            continue;
        }

        s = s.substr(i);
        length += i;

        // Now, parse for the truth body
        ParsedPrgms lists = parsePemdas(s, ends);
        
        while (!lists->isEmpty()) {
            parsed_prgm lst = lists->remove(0);
            lst.len += length;
            
            lst.item = new MapExp(fun.item, lst.item);
            res->add(0, lst);

            fun.item = fun.item->clone();
        }
        delete lists;
    }
    delete functions;

    return res;
}

ParsedPrgms parseExplicitThunk(string str, bool ends) {
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    int len = parseLit(str, "thunk");
    if (len < 0) return res;
    else str = str.substr(len);

    int i = parseLit(str, "{");
    if (i < 0) return res;
    else {
        len += i;
        str = str.substr(i);
    }

    ParsedPrgms exps = parsePemdas(str, false);
    
    while (!exps->isEmpty()) {
        parsed_prgm p = exps->remove(0);
        string s = str.substr(p.len);

        p.len += len;

        if ((i = parseLit(s, "}")) < 0) {
            delete p.item;
            continue;
        } else {
            p.len += i;
            s = s.substr(i);
        }

        if (ends && parseSpaces(s) == s.length()) {
            delete p.item;
            continue;
        } else {
            // Convert the expression to a thunk and add it to the result
            p.item = new ThunkExp(p.item);
            res->add(0, p);
        }
    }

    return res;
}

/**
 * top-level pemdas-exp
 */
ParsedPrgms parsePemdas(string str, bool ends) {
    // Parse for lambdas
    ParsedPrgms res;
    
    res = parseLambdaExp(str, ends);
    if (!res->isEmpty()) return res;
    
    // Parse for map expression
    delete res;
    res = parseMapExp(str, ends);
    if (!res->isEmpty()) return res;

    // Parse for fold expression
    delete res;
    res = parseFoldExp(str, ends);
    if (!res->isEmpty()) return res;

    // Parse for derivative
    delete res;
    res = parseDerivative(str, ends);
    if (!res->isEmpty()) return res;

    // Parse for an explicitly defined thunk
    delete res;
    res = parseExplicitThunk(str, ends);
    if (!res->isEmpty()) return res;
    
    // Parse for calculation
    delete res;
    res = parseSetExp(str, ends);

    return res;
}

ParsedPrgms parsePrintExp(string str, bool ends) {
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    int i;
    int len = parseLit(str, "print");
    if (len < 0) return res;

    str = str.substr(len);

    LinkedList<struct arglist> *arglists = parseArgList(str, ends);
    while (!arglists->isEmpty()) {
        struct arglist alst = arglists->remove(0);

        Expression **args = new Expression*[alst.list->size() + 1];
        for (i = 0; !alst.list->isEmpty(); i++)
            args[i] = alst.list->remove(0).exp;
        args[i] = NULL;
        delete alst.list; // The list is empty and useless (destroy it)

        parsed_prgm p;
        p.item = new PrintExp(args);
        p.len = alst.len + len;

        res->add(0, p);
    }
    delete arglists;
    
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
            
            body.item = new WhileExp(cond.item->clone(), body.item);
            body.len += length;
            
            res->add(0, body);
        }
        delete bodies;
        delete cond.item;
    }

    delete conditions;
    return res;
}

