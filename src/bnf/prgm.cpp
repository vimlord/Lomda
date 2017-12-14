#include "bnf.hpp"
#include "expression.hpp"

using namespace std;

ParsedPrgms parseStatement(string str, bool ends) {
    ////std::cout << "Searching for statement in '" << str << "'\n";
    ParsedPrgms tmp;
    
    // parse pemdas-exp
    ParsedPrgms res = parsePemdas(str, ends);    
    
    // if-exp
    tmp = parseIfExp(str, ends);
    while (!tmp->isEmpty()) res->add(0, tmp->remove(0));
    delete tmp;

    // while-exp
    tmp = parseWhileExp(str, ends);
    while (!tmp->isEmpty()) res->add(0, tmp->remove(0));
    delete tmp;

    // for-exp
    tmp = parseForExp(str, ends);
    while (!tmp->isEmpty()) res->add(0, tmp->remove(0));
    delete tmp;

    // insert-exp
    tmp = parseInsertExp(str, ends);
    while (!tmp->isEmpty()) res->add(0, tmp->remove(0));
    delete tmp;

    // remove-exp
    tmp = parseRemoveExp(str, ends);
    while (!tmp->isEmpty()) res->add(0, tmp->remove(0));
    delete tmp;
    
    return res;
}


ParsedPrgms parseSequence(string str, bool ends) {
    // Parse the first line
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    ParsedPrgms prgms = parseStatement(str, false);

    while (!prgms->isEmpty()) {
        parsed_prgm left = prgms->remove(0);
        string s = str.substr(left.len);
        
        int i = parseLit(s, ";");
        if (i < 0) {
            delete left.item;
            continue;
        }
        s = s.substr(i);
        left.len += i;
        
        ParsedPrgms posts = parseProgram(s, ends);
        while (!posts->isEmpty()) {
            // Check right hand side of semicolon
            parsed_prgm right = posts->remove(0);
            right.len += left.len;
            right.item = new SequenceExp(left.item, right.item);
            res->add(0, right);
        }
        delete posts;

    }
    delete prgms;

    return res;
}

/**
 * <program> ::= <let-exp> | <sequence-exp> | <statement>
 */
ParsedPrgms parseProgram(string str, bool ends) {
    ParsedPrgms res = new LinkedList<parsed_prgm>;
    ParsedPrgms prgms;
    Iterator<int, parsed_prgm> *it;

    // First, parse for a let expression
    prgms = parseLetExp(str, ends);
    it = prgms->iterator();
    while (it->hasNext())
        res->add(0, it->next());
    delete it;
    
    // Then, we will simply parse for statements
    prgms = parseStatement(str, ends);
    it = prgms->iterator();
    while (it->hasNext())
        res->add(0, it->next());
    delete it;
    
    prgms = parseSequence(str, ends);
    it = prgms->iterator();
    while (it->hasNext())
        res->add(0, it->next());
    delete it;

    return res;

}

