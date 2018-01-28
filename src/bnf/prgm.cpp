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
        int i;
        
        if ((i = parseLit(s, ";")) < 0) {
            // End of program
            if (!ends || parseSpaces(s) == s.length())
                res->add(0, left);
            continue;
        } else do {
            // Possibly another statement. So, get ready
            left.len += i;
            s = s.substr(i);
        } while ((i = parseLit(s, ";")) >= 0);

        if (!ends) {
            // Perhaps not the end of th program
            res->add(0, left);
        } else if (parseSpaces(s) == s.length()) {
            // End of program
            res->add(0, left);
            continue;
        }
        
        ParsedPrgms posts = parseProgram(s, ends);
        while (!posts->isEmpty()) {
            // Check right hand side of semicolon
            parsed_prgm right = posts->remove(0);
            right.len += left.len;

            if (typeid(*(right.item)) == typeid(SequenceExp))
                // The right side is also a sequence, so we can add the left side to
                // that sequence.
                ((SequenceExp*) right.item)->getSeq()->add(0, left.item->clone());
            else {
                auto seq = new LinkedList<Exp>;
                seq->add(0, right.item);
                seq->add(0, left.item->clone());

                right.item = new SequenceExp(seq);
            }
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

