#include "expression.hpp"

bool DictExp::isNormalForm(int rank) {
    // All primitives are LNF
    auto it = vals->iterator();
    while (it->hasNext())
        if (!it->next()->isNormalForm(rank)) {
            delete it;
            return false;
        }
    delete it;
    return true;
}

bool LambdaExp::isNormalForm(int rank) {
    if (rank == 0) return true;

    return exp->isNormalForm(rank);
}

bool ListExp::isNormalForm(int rank) {
    // All primitives are LNF
    auto it = list->iterator();
    while (it->hasNext())
        if (!it->next()->isNormalForm(rank)) {
            delete it;
            return false;
        }
    delete it;
    return true;
}

