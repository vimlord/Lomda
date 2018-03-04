#ifndef _EXPRESSIONS_LIST_HPP_
#define _EXPRESSIONS_LIST_HPP_

#include "expression.hpp"

// For accessing an element of an array (theoretically, can be used to get or set)
class ListAccessExp : public Expression {
    private:
        Exp list;
        Exp idx;
    public:
        ListAccessExp(Exp x, Exp i) : list(x), idx(i) {}
        ~ListAccessExp() { delete list; delete idx; }

        Val evaluate(Env);
        Val derivativeOf(std::string, Env, Env);
        Type* typeOf(Tenv);

        Exp clone() { return new ListAccessExp(list->clone(), idx->clone()); }
        std::string toString();

        Exp getList() { return list; }
        Exp getIdx() { return idx; }

        Exp optimize();
};

// For adding to a list
class ListAddExp : public Expression {
    private:
        Exp list;
        Exp idx;
        Exp elem;
    public:
        ListAddExp(Exp x, Exp i, Exp e) : list(x), idx(i), elem(e) {}
        ~ListAddExp() { delete list; delete idx; delete elem; }

        Val evaluate(Env);

        Exp clone() { return new ListAddExp(list->clone(), idx->clone(), elem->clone()); }
        std::string toString();
};

// For removing from a list
class ListRemExp : public Expression {
    private:
        Exp list;
        Exp idx;
    public:
        ListRemExp(Exp x, Exp i) : list(x), idx(i) {}
        ~ListRemExp() { delete list; delete idx; }

        Val evaluate(Env);

        Exp clone() { return new ListRemExp(list->clone(), idx->clone()); }
        std::string toString();
};

// For removing from a list
class ListSliceExp : public Expression {
    private:
        Exp list;
        Exp from;
        Exp to;
    public:
        ListSliceExp(Exp x, Exp i, Exp j) : list(x), from(i), to(j) {}
        ~ListSliceExp() { delete list; delete from; delete to; }

        Val evaluate(Env);
        Type* typeOf(Tenv);

        Exp clone() {
            Exp f = from ? from->clone() : NULL;
            Exp t = to ? to->clone() : NULL;
            return new ListSliceExp(list->clone(), f, t); }
        std::string toString();
};

#endif
