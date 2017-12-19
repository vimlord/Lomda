#include "bnf.hpp"
#include "config.hpp"

using namespace std;

List<Expression*>* parse(string str) {
    List<Expression*> *outcomes = new LinkedList<Expression*>;
    ParsedPrgms prgms = parseProgram(str, true);

    Iterator<int, parsed_prgm> *it = prgms->iterator();
    while (it->hasNext())
        outcomes->add(0, it->next().item);
    
    return outcomes;
}

Expression* compile(string str) {
    List<Expression*> *res = parse(str);

    if (res->size() == 0) {
        // There were no possible interpretations; display error and return
        cerr << "\x1b[31m\x1b[1merror:\x1b[0m "
            << "could not generate syntax tree from given program\n";
        delete res;
        return NULL;
    } else if (res->size() > 1) {
        // The given program is ambiguous; display error if werror enabled
        if (werror)
            cerr << "\x1b[31m\x1b[1merror:\x1b[0m ";
        else
            cerr << "\x1b[33m\x1b[1mwarning:\x1b[0m ";

        cerr << "syntax tree of given program is ambiguous (found " << res->size() << " versions)\n";
        
        // We will return the last interpretation of the program if we allow
        // an ambiguous statement to be parsed
        Expression *exp = werror ? NULL : res->remove(0);
        while (!res->isEmpty()) {
            Expression *e = res->remove(0);
            std::cout << *e << "\n";
            delete e;
        }
        delete res;
        return exp;
    } else {
        // There is only one interpretation; returning it
        Expression *exp = res->remove(0);
        //cout << "\x1b[1mparsed:\x1b[0m " << *exp << "\n";
        delete res;
        return exp;
    }
}

