#include "proof.hpp"

void show_proof_step(std::string x) {
    if (VERBOSITY()) std::cout << "\x1b[3m" << x << "\x1b[0m\n";
}
void show_proof_therefore(std::string x) {
    show_proof_step("Therefore, " + x + ".");
}
void show_mgu_step(Tenv t, Type *a, Type *b, Type *c) {
    if (c)
        show_proof_step("Under " + t->toString() + ", " + a->toString() + " = " + b->toString() + " unifies to " + c->toString() + ".");
    else
        show_proof_step("Under " + t->toString() + ", " + a->toString() + " = " + b->toString() + " is not unifiable.");
}

std::string type_res_str(Tenv tenv, Exp exp, Type *type) {
    std::string env = tenv ? tenv->toString() : "{}";
    if (type)
        return env + " ⊢ " + exp->toString() + " : " + type->toString();
    else
        return env + " ⊢ " + exp->toString() + " is untypable";
}
