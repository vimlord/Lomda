#include "bnf.hpp"
#include "expression.hpp"

using namespace std;

ParsedPrgms parseStatement(string str, bool ends) {
    // We only need to find a single set of working paths. All but
    // pemdas-exp begin with a keyword, hence only one of these
    // calls should return a value.
    ParsedPrgms res;
    
    // if-exp
    res = parseIfExp(str, ends);
    if (!res->isEmpty()) return res;

    // while-exp
    delete res;
    res = parseWhileExp(str, ends);
    if (!res->isEmpty()) return res;

    // for-exp
    delete res;
    res = parseForExp(str, ends);
    if (!res->isEmpty()) return res;

    // print-exp
    delete res;
    res = parsePrintExp(str, ends);
    if (!res->isEmpty()) return res;

    // insert-exp
    delete res;
    res = parseInsertExp(str, ends);
    if (!res->isEmpty()) return res;

    // remove-exp
    delete res;
    res = parseRemoveExp(str, ends);
    if (!res->isEmpty()) return res;

    // pemdas-exp
    delete res;
    res = parsePemdas(str, ends);    

    return res;
}

/**
 * <sequence-exp> ::= <statement> ';' <program> | <statement>
 */
ParsedPrgms parseSequence(string str, bool ends) {
    // Parse the first line
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    ParsedPrgms prgms = parseStatement(str, false);

    while (!prgms->isEmpty()) {
        parsed_prgm left = prgms->remove(0);
        string s = str.substr(left.len);
        
        int i = parseLit(s, ";");
        if (i < 0) {
            if (!ends || parseSpaces(s) == s.length())
                res->add(0, left);
            else
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
 * <program> ::= <let-exp> | <sequence-exp>
 */
ParsedPrgms parseProgram(string str, bool ends) {
    ParsedPrgms prgms;

    // First, parse for a let expression
    prgms = parseLetExp(str, ends);
    if (prgms->isEmpty()) {
        delete prgms;
        prgms = parseSequence(str, ends);
    }
    return prgms;

}

