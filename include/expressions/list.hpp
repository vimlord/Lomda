#ifndef _EXPRESSIONS_LIST_HPP_
#define _EXPRESSIONS_LIST_HPP_

// For accessing an element of an array (theoretically, can be used to get or set)
class ListAccessExp : public Differentiable {
    private:
        Expression *list;
        Expression *idx;
    public:
        ListAccessExp(Expression* x, Expression* i) : list(x), idx(i) {}
        ~ListAccessExp() { delete list; delete idx; }

        Value* valueOf(Environment*);
        Value* derivativeOf(std::string, Environment*, Environment*);

        Expression* clone() { return new ListAccessExp(list->clone(), idx->clone()); }
        std::string toString();
};

// For adding to a list
class ListAddExp : public Expression {
    private:
        Expression *list;
        Expression *idx;
        Expression *elem;
    public:
        ListAddExp(Expression *x, Expression *i, Expression *e) : list(x), idx(i), elem(e) {}
        ~ListAddExp() { delete list; delete idx; delete elem; }

        Value* valueOf(Environment*);

        Expression* clone() { return new ListAddExp(list->clone(), idx->clone(), elem->clone()); }
        std::string toString();
};

// For removing from a list
class ListRemExp : public Expression {
    private:
        Expression *list;
        Expression *idx;
    public:
        ListRemExp(Expression* x, Expression* i) : list(x), idx(i) {}
        ~ListRemExp() { delete list; delete idx; }

        Value* valueOf(Environment*);

        Expression* clone() { return new ListRemExp(list->clone(), idx->clone()); }
        std::string toString();
};

// For removing from a list
class ListSliceExp : public Expression {
    private:
        Expression *list;
        Expression *from;
        Expression *to;
    public:
        ListSliceExp(Expression* x, Expression* i, Expression* j) : list(x), from(i), to(j) {}
        ~ListSliceExp() { delete list; delete from; delete to; }

        Value* valueOf(Environment*);

        Expression* clone() { return new ListSliceExp(list->clone(), from->clone(), to->clone()); }
        std::string toString();
};

#endif
