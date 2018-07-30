#include "stdlib.hpp"

#include "expression.hpp"
#include <cmath>

auto std_range = [](Env env) {
    Val a = env->apply("a");
    Val b = env->apply("b");

    if (!isVal<IntVal>(a) || !isVal<IntVal>(b)) {
        throw_err("type", "list.range : Z -> Z -> [Z] cannot be applied to arguments " + a->toString() + " and " + b->toString());
        return (Val) NULL;
    }

    int i = ((IntVal*) a)->get();
    int j = ((IntVal*) b)->get();

    auto range = new ListVal;
    while (i < j) {
        range->add(range->size(), new IntVal(i++));
    }

    return (Val) range;
};

Type* type_stdlib_list() {
    return new DictType {
        {"range",
            new LambdaType("a",
                new IntType,
                new LambdaType("b",
                    new IntType,
                    new ListType(new RealType)))
        }
    };
}

Val load_stdlib_list() {
    return new DictVal {
        {"range",
            new LambdaVal(new std::string[3]{"a", "b", ""},
                (new ImplementExp(std_range, NULL))
                    ->setName("range(a, b)"))
        }
    };
}


