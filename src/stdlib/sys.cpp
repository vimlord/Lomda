#include "stdlib.hpp"
#include "expressions/stdlib.hpp"

#include "expression.hpp"

auto stdlib_exit = [](Env env) {
    Val v = env->apply("x");

    if (isVal<IntVal>(v))
        exit(((IntVal*) v)->get());
    else {
        throw_err("type", "sys.exit : Z -> void cannot be applied to " + v->toString());
        return (Val) NULL;
    }
};

Type* type_stdlib_sys() {
    return new DictType {
        { "exit", new LambdaType("x", new IntType, new VoidType) },
        { "argv", new ListType(new StringType) },
        { "stdin", new IntType },
        { "stdout", new IntType },
        { "stderr", new IntType }
    };
}

Val load_stdlib_sys() {
    
    // Argument vector
    char **args = configuration.argv;
    ListVal *argv = new ListVal;

    // Assign items to the list
    for (int i = 0; args[i]; i++) {
        StringVal *arg = new StringVal(args[i]);
        argv->add(i, arg);
    }

    return new DictVal {
        { 
            "exit",
            new LambdaVal(new std::string[2]{"x", ""},
                new ImplementExp(stdlib_exit, NULL))
        }, { "argv", argv },
        { "stdin", new IntVal(0) },
        { "stdout", new IntVal(1) },
        { "stderr", new IntVal(2) }
    };
}


