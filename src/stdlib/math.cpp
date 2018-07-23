#include "stdlib.hpp"

#include "expression.hpp"

#include <cmath>

auto std_mathfn = [](Env env, bool (*fn)(float)) {
    Val v = env->apply("x");
    
    if (isVal<RealVal>(v)) {
        float f = ((RealVal*) v)->get();
        return (Val) new BoolVal(fn(f));
    } else
        return (Val) new BoolVal(false);
};

auto std_isinfinite = [](Env env) {
    return (Val) std_mathfn(env, std::isinf);
};

auto std_isfinite = [](Env env) {
    return (Val) std_mathfn(env, std::isfinite);
};

auto std_isnan = [](Env env) {
    return (Val) std_mathfn(env, std::isnan);
};

Type* type_stdlib_math() {
    return new DictType {
        {
            "isinfinite",
            new LambdaType("x",
                new RealType, new BoolType)
        }, {
            "isfinite",
            new LambdaType("x",
                new RealType, new BoolType)
        }
    };
}

Val load_stdlib_math() {
    return new DictVal {
        {
            "isinfinite",
            new LambdaVal(new std::string[2]{"x", ""},
                new ImplementExp(std_isinfinite, NULL))
        }, {
            "isfinite",
            new LambdaVal(new std::string[2]{"x", ""},
                new ImplementExp(std_isfinite, NULL))
        }, {
            "isnan",
            new LambdaVal(new std::string[2]{"x", ""},
                new ImplementExp(std_isnan, NULL))
        }
    };
}

