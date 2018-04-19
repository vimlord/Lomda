#include "bnf.hpp"

#include "types.hpp"

using namespace std;

parsed_type parseType(std::string str, bool ends) {
    parsed_type p;

    int i;
    if ((i = parseLit(str, "B")) > 0) {
        p.len = i;
        p.item = new BoolType;
    } else if ((i = parseLit(str, "R")) > 0) {
        p.len = i;
        p.item = new RealType;
    } else if ((i = parseLit(str, "S")) > 0) {
        p.len = i;
        p.item = new StringType;
    } else if ((i = parseLit(str, "Z")) > 0) {
        p.len = i;
        p.item = new IntType;
    } else if ((i = parseLit(str, "(")) > 0) {
        string s = str.substr(i);
        p.len = i;

        parsed_type a = parseType(s, false);
        if (!a.item) {
            p.len = -1;
            p.item = NULL;
        } else {
            p.len += a.len;
            s = s.substr(a.len);

            int type;
            if ((i = parseLit(s, "->") > 0)) {
                type = 0;
            } else if ((i = parseLit(s, "*")) > 0) {
                type = 1;
            } else type = -1;

            if (type >= 0) {
                p.len += i;
                s = s.substr(i);
                
                parsed_type b = parseType(s, false);
                if (!b.item) {
                    delete a.item;
                    p.item = NULL;
                    p.len = -1;
                } else {
                    p.len += b.len;
                    s = s.substr(b.len);

                    if (type == 0)
                        p.item = new LambdaType("", a.item, b.item);
                    else
                        p.item = new TupleType(a.item, b.item);

                    if ((i = parseLit(s, ")")) < 0) {
                        delete p.item;
                        p.item = NULL;
                        p.len = -1;
                    } else
                        p.len += i;
                }

            } else {
                delete a.item;
                p.item = NULL;
                p.len = -1;
            }

        }
    } else {
        p.len = -1;
        p.item = NULL;
    }

    if (ends && parseSpaces(str.substr(p.len)) < str.length()) {
        p.item = NULL;
        p.len = -1;
    }

    return p;
}

