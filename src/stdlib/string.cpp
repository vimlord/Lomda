#include "stdlib.hpp"
#include "expressions/stdlib.hpp"

#include "types.hpp"

#include <string>

using namespace std;

Val std_strcat(Env env) {
    Val A = env->apply("a");
    Val B = env->apply("b");
    return new StringVal(A->toString() + B->toString());
}

Val std_substring(Env env) {
    Val S = env->apply("str");
    int i = ((IntVal*) env->apply("i"))->get();
    int j = ((IntVal*) env->apply("j"))->get();
    return new StringVal(S->toString().substr(i, j));
}

Val std_strstr(Env env) {
    Val S = env->apply("a");
    Val T = env->apply("b");
    
    return new IntVal(S->toString().find(T->toString()));
}

Type* type_stdlib_string() {
    return new DictType {
        { // strcat : S -> S -> S
            "strcat",
            new LambdaType("a",
                new StringType,
                new LambdaType("b",
                    new StringType,
                    new StringType
                )
            )
        }, {
            "substring",
            new LambdaType("str",
                new StringType,
                new LambdaType("i",
                    new IntType,
                    new LambdaType("j",
                        new IntType,
                        new StringType
            )))
        }, {
            "strstr",
            new LambdaType("a",
                new StringType,
                new LambdaType("b",
                    new StringType,
                    new IntType
                )
            )
        }
    };
}

Val load_stdlib_string() {
    return new DictVal {
        { // strcat(a,b) = a | b
            "strcat",
            new LambdaVal(new std::string[3]{"a", "b", ""},
                new ImplementExp(std_strcat, NULL)
            )
        }, { // substring(str, i, j) = substring
            "substring",
            new LambdaVal(new std::string[4]{"str", "i", "j", ""},
                new ImplementExp(std_substring, NULL)
            )
        }, {
            "strstr",
            new LambdaVal(new std::string[3]{"a", "b", ""},
                new ImplementExp(std_strstr, NULL)
            )
        }
    };
}

