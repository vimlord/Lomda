#include "stringable.hpp"

#include "expression.hpp"
#include "value.hpp"
#include "baselang/environment.hpp"

using namespace std;

std::ostream &operator<< (std::ostream &os, Stringable &obj) {
    os << obj.toString();
    return os;
}

/* EXPRESSIONS */
string AdtExp::toString() {
    string str = name + "<" + kind + ">";
    if (!args[0]) return str;
    
    str += "(";
    for (int i = 0; args[i]; i++) {
        if (i) str += ", ";
        str += args[i]->toString();
    }
    
    return str + ")";
}
string AdtDeclarationExp::toString() {
    string str = "type " + name + " = ";

    for (int i = 0; argss[i]; i++) {
        string kind = ids[i];
        Type **args = argss[i];

        if (i) str += " | ";
        
        str += kind + "(";

        for (int j = 0; args[j]; j++) {
            if (j) str += ", ";
            str += args[j]->toString();
        }

        str += ")";
    }
    
    str += "; " + body->toString();

    return str;
}
string AndExp::toString() {
    return left->toString() + " and " + right->toString();
}
string ApplyExp::toString() {
    string s = (isExp<VarExp>(op))
        ? op->toString() 
        : ("(" + op->toString() + ")");
    s += "(";
    for (int i = 0; args[i]; i++) {
        if (i) s += ", ";
        s += args[i]->toString();
    }

    s += ")";

    return s;
}
string CastExp::toString() {
    return exp->toString() + " as " + type->toString();
}
string DerivativeExp::toString() {
    if (isExp<VarExp>(func))
        return "d" + func->toString() + "/d" + var;

    string s = "d/d" + var + " (" + func->toString() + ")";
    return s;
}
string DictVal::toString() {
    string s = "{";

    auto vt = vals->iterator();

    for (bool b = false; vt->hasNext(); b = true) {
        if (b) s += ", ";
        auto k = vt->next();
        auto v = vals->get(k);
        s += k + " : " + v->toString();
    }

    delete vt;

    s += "}";

    return s;
}
string DiffExp::toString() {
    return left->toString() + " - " + right->toString();
}
string DivExp::toString() {
    return left->toString() + " / " + right->toString();
}
string CompareExp::toString() {
    string s = left->toString();
    
    switch (operation) {
        case EQ:
            s += "=="; break;
        case NEQ:
            s += "!="; break;
        case GT:
            s += ">"; break;
        case LT:
            s += "<"; break;
        case GEQ:
            s += ">="; break;
        case LEQ:
            s += "<="; break;
    }

    return s + right->toString();
}
string ExponentExp::toString() {
    return left->toString() + " ^ " + right->toString();
}
string FoldExp::toString() {
    return "fold " + list->toString() + " into " + func->toString() + " from " + base->toString();
}
string ForExp::toString() {
    return "for " + id + " in " + set->toString() + " " + body->toString();
}
string HasExp::toString() {
    return item->toString() + " in " + set->toString();
}
string IfExp::toString() {
    return "if " + cond->toString() + " then " + tExp->toString() + " else " + fExp->toString();
}
string ImportExp::toString() {
    return "import " + module + (name == module ? "" : (" as " + name)) + (exp ? ("; " + exp->toString()) : "");
}
string IntExp::toString() {
    return to_string(val);
}
string IsaExp::toString() {
    return exp->toString() + " isa " + type->toString();
}
string LambdaExp::toString() {
    string s = "lambda ("; 

    for (int i = 0; xs[i] != ""; i++) {
        if (i) s += ", ";
        s += xs[i];
    }

    s += ") ";
    
    if (isExp<LetExp>(exp) || isExp<SequenceExp>(exp))
        s += "(" + exp->toString() + ")";
    else
        s += exp->toString();

    return s;
}
string LetExp::toString() {
    string s = "let ";
    
    for (int i = 0; exps[i]; i++) {
        if (i) s += ", ";
        s += ids[i] + " = " + exps[i]->toString();
    }
    s += "; " + body->toString();
    return s;
}
string ListExp::toString() {
    string s = "[";

    auto it = list->iterator();

    for (int i = 0; it->hasNext(); i++) {
        if (i) s += ", ";
        s += it->next()->toString();
    }

    s += "]";

    delete it;
    return s;

}

string DictAccessExp::toString() {
    return list->toString() + "." + idx;
}
string ListAccessExp::toString() {
    return list->toString() + "[" + idx->toString() + "]";
}
string ListAddExp::toString() {
    return "insert " + elem->toString()
         + " into " + list->toString()
         + " at " + idx->toString();
}
string ListRemExp::toString() {
    return "remove " + idx->toString()
         + " from " + list->toString();
}
string ListSliceExp::toString() {
    return list->toString() + "["
            + (from ? from->toString() : "")
            + ":"
            + (to ? to->toString() : "") + "]";
}
string MagnitudeExp::toString() {
    return "|" + exp->toString() + "|";
}
string MapExp::toString() {
    return "map " + func->toString() + " over " + list->toString();
}
string ModulusExp::toString() {
    return left->toString() + " mod " + right->toString();
}
string MultExp::toString() {
    return left->toString() + " * " + right->toString();
}
string NormExp::toString() {
    return "||" + exp->toString() + "||";
}
string NotExp::toString() {
    return "not " + exp->toString();
}
string OrExp::toString() {
    return left->toString() + " or " + right->toString();
}
string PrintExp::toString() {
    string s = "print";
    for (int i = 0; args[i]; i++)
        s += " " + args[i]->toString();

    return s;
}
string RealExp::toString() {
    return to_string(val);
}
string SequenceExp::toString() {
    string s = "";

    auto it = seq->iterator();
    for (int i = 0; it->hasNext(); i++) {
        if (i) s += "; ";
        s += it->next()->toString();
    }
    
    return s;
}
string SetExp::toString() {
    return tgt->toString() + " = " + exp->toString();
}
string StdMathExp::toString() {
    string s;
    switch (fn) {
        case SIN:
            s = "sin(";
            break;
        case COS:
            s = "cos(";
            break;
        case TAN:
            s = "tan(";
            break;
        case ASIN:
            s = "asin(";
            break;
        case ACOS:
            s = "acos(";
            break;
        case ATAN:
            s = "atan(";
            break;
        case LOG:
            s = "log(";
            break;
        case SQRT:
            s = "sqrt(";
            break;
        case EXP:
            s = "exp(";
            break;
        case MIN:
            s = "min(";
            break;
        case MAX:
            s = "max(";
            break;
        default:
            s = "undefined";
            return s;
    }
    
    s += e->toString() + ")";
    return s;
}
string StringExp::toString() {
    return "\"" + val + "\"";
}
string SwitchExp::toString() {
    string str = "switch " + adt->toString() + " in ";
    for (int i = 0; names[i] != ""; i++) {
        if (i) str += " | ";
        str += names[i] + "(";
        for (int j = 0; idss[i][j] != ""; j++) {
            if (j) str += ", ";
            str += idss[i][j];
        }
        str += ") -> " + bodies[i]->toString();
    }
    return str;
}
string SumExp::toString() {
    return left->toString() + " + " + right->toString();
}
string ThunkExp::toString() {
    return "thunk { " + exp->toString() + " }";
}
string TupleExp::toString() { return "(" + left->toString() + ", " + right->toString() + ")"; }
string TupleAccessExp::toString() {
    if (idx)
        return "right of " + exp->toString();
    else
        return "left of " + exp->toString();
}
string ValExp::toString() {
    return "antithunk { " + val->toString() + " }";
}
string VarExp::toString() {
    return id;
}
string WhileExp::toString() {
    if (alwaysEnter)
        return "do {\n" + body->toString() + "\n} while (" + cond->toString() + ")";
    else
        return "while (" + cond->toString() + ") {\n" + body->toString() + "\n}";
}

/* ENVIRONMENTS */

string Environment::toString() {
    string s = "{";
    for (auto it : store) {
        auto id = it.first;
        auto ref = it.second;

        s += id += " := ";

        if (isVal<LambdaVal>(ref))
            s += "λ";
        else s += ref->toString();

        s += ", ";
    }


    if (subenv)
        return s + subenv->toString().substr(1);
    else
        return s.substr(0, s.length() - 2) + "}";
    
    return s;
}

/* Type env */
string TypeEnv::toString() {
    string s = "";

    int i = 0;
    for (auto it : mgu) {
        string v = it.second->toString();
        //if (v != it.first) {
            if (s.length() == 0) s = "{";
            if (i++) s += ", ";
            s += v + "/" + it.first;
        //}
    }
    if (s.length() > 0) s += "}";

    i = 0;
    s += "{";
    for (auto it : types) {
        if (i++) s += ", ";
        s += it.first + " : " + it.second->toString();
    }
    s += "}";
    
    return s;
}

/* VALUES */

string AdtVal::toString() {
    string str = type + "<" + kind + ">(";
    for (int i = 0; args[i]; i++) {
        if (i) str += ", ";
        str += args[i]->toString();
    }

    return str + ")";
}
string BoolVal::toString() { return val ? "true" : "false"; }
string DictExp::toString() {
    string s = "{";

    auto kt = keys->iterator();
    auto vt = vals->iterator();

    for (bool b = false; kt->hasNext(); b = true) {
        if (b) s += ", ";
        s += kt->next() + " : " + vt->next()->toString();
    }

    delete kt;
    delete vt;

    s += "}";

    return s;
}
string IntVal::toString() { return to_string(val); }
string LambdaVal::toString() {
    string s =  "λ";

    for (int i = 0; xs[i] != ""; i++) {
        if (i) s += ",";
        s += xs[i];
    }
    
    s += "." + exp->toString();
    s += " | " + (env ? env->toString() : "{}");

    return s;
}
string ListVal::toString() {
    string s = "[";

    for (int i = 0; i < list->size(); i++) {
        if (i) s += ", ";
        Val v = list->get(i);
        if (v)
            s += v->toString();
        else
            s += "null";
    }

    s += "]";
    return s;
}
string RealVal::toString() { return to_string(val); }
string StringVal::toString() { return val; }
string Thunk::toString() { return val ? val->toString() : ("" + exp->toString() + " | " + env->toString()); }
string TupleVal::toString() { return "(" + left->toString() + ", " + right->toString() + ")"; }

string AlgebraicDataType::toString() {
    return "ADT<" + name + ">";
}
string LambdaType::toString() {
    return "(" + left->toString() + " -> " + right->toString() + ")";
}
string ListType::toString() { return "[" + type->toString() + "]"; }
string TupleType::toString() { return "(" + left->toString() + " * " + right->toString() + ")"; }
string SumType::toString() { return "(" + left->toString() + " + " + right->toString() + ")"; }
string MultType::toString() { return "(" + left->toString() + " x " + right->toString() + ")"; }
string DerivativeType::toString() { return "d" + left->toString() + "/d" + right->toString(); }
string DictType::toString() {
    string s = "{";
    auto tt = types->iterator();
    while (tt->hasNext()) {
        string id = tt->next();
        s += id + " : " + types->get(id)->toString();
        if (tt->hasNext()) s += ", ";
    }
    delete tt;

    return s + "}";
}


