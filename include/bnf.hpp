#ifndef _BNF_HPP_
#define _BNF_HPP_

#include "structures/list.hpp"
#include "baselang/expression.hpp"

// The output structure from parsing
template <typename T>
struct parsed {
    int len;
    T item;
};
typedef struct parsed<
    Expression*
> parsed_prgm;
typedef List<parsed_prgm>* ParsedPrgms;

typedef struct parsed<std::string> parsed_id;
typedef struct parsed<int> parsed_int;

// Argument types for the let expression
struct arg {
    std::string id;
    Expression *exp;
};
struct arglist {
    int len;
    LinkedList<struct arg> *list;
};

parsed_id parseId(std::string);
parsed_int parseInt(std::string);

int parseSpaces(std::string);
int parseLit(std::string, std::string);

LinkedList<struct arglist>* parseArgList(std::string, bool = true);

ParsedPrgms parseCodeBlock(std::string, bool = true);

ParsedPrgms parseMultiplicative(std::string, bool = true);
ParsedPrgms parseAdditive(std::string, bool = true);

ParsedPrgms parseAndExp(std::string, bool = true);
ParsedPrgms parseEquality(std::string, bool = true);
ParsedPrgms parseNotExp(std::string, bool = true);

ParsedPrgms parsePrimitive(std::string str, bool = true);
ParsedPrgms parseListExp(std::string, bool = true);

ParsedPrgms parseForExp(std::string str, bool = true);
ParsedPrgms parseIfExp(std::string str, bool = true);
ParsedPrgms parseWhileExp(std::string str, bool = true);

ParsedPrgms parsePrintExp(std::string str, bool = true);

ParsedPrgms parseInsertExp(std::string str, bool = true);
ParsedPrgms parseRemoveExp(std::string str, bool = true);

ParsedPrgms parseApplyExp(std::string, bool = true);
ParsedPrgms parseLetExp(std::string, bool = true);
ParsedPrgms parseSetExp(std::string, bool = true);
ParsedPrgms parsePemdas(std::string, bool = true);

ParsedPrgms parseLambdaExp(std::string, bool = true);
ParsedPrgms parseDerivative(std::string, bool = true);

ParsedPrgms parseProgram(std::string, bool = true);
ParsedPrgms parseStatement(std::string, bool = true);

Expression* compile(std::string);
List<Expression*>* parse(std::string);

#endif
