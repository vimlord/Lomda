#include "stdlib.hpp"

#include "stdlib.hpp"
#include "expressions/stdlib.hpp"

#include "types.hpp"

#include <cstdlib>
#include <cmath>

Val std_rand_uniform(Env env) {
    Val A = env->apply("a");
    if (!val_is_number(A)) {
        throw_err("type", "random.uniform : R -> R -> R cannot take parameter " + A->toString());
        return NULL;
    }

    Val B = env->apply("b");
    if (!val_is_number(B)) {
        throw_err("type", "random.uniform : R -> R -> R cannot take parameter " + B->toString());
        return NULL;
    }

    float x = (1.0 * rand()) / RAND_MAX;

    x *=
        (isVal<IntVal>(B)
            ? ((IntVal*) B)->get()
            : ((RealVal*) B)->get())
        -
        (isVal<IntVal>(A)
            ? ((IntVal*) A)->get()
            : ((RealVal*) A)->get());
    
    x += isVal<IntVal>(A)
        ? ((IntVal*) A)->get()
        : ((RealVal*) A)->get();
    
    return new RealVal(x);
}

Val std_rand_normal(Env env) {
    Val A = env->apply("a");
    if (!val_is_number(A)) {
        throw_err("type", "random.uniform : R -> R -> R cannot take parameter " + A->toString());
        return NULL;
    }

    Val B = env->apply("b");
    if (!val_is_number(B)) {
        throw_err("type", "random.uniform : R -> R -> R cannot take parameter " + B->toString());
        return NULL;
    }

    // Two uniform variables.
    float a = (1.0 * rand()) / RAND_MAX;
    float b = (1.0 * rand()) / RAND_MAX;

    // Thus, we can generate a normal variable.
    float x = sqrt(-2 * log(a)) * sin(2 * M_PI * b);
    
    // Standard deviation
    x *=
        sqrt(isVal<IntVal>(B)
            ? ((IntVal*) B)->get()
            : ((RealVal*) B)->get());
    
    x += isVal<IntVal>(A)
        ? ((IntVal*) A)->get()
        : ((RealVal*) A)->get();
    
    return new RealVal(x);
}

Type* type_stdlib_random() {
    return new DictType {
        { // uniform : R -> R -> R
            "uniform",
            new LambdaType("a",
                new RealType,
                new LambdaType("b",
                    new RealType,
                    new RealType))
        }, { // uniform : R -> R -> R
            "normal",
            new LambdaType("a",
                new RealType,
                new LambdaType("b",
                    new RealType,
                    new RealType))
        }
    };
}

Val load_stdlib_random() {
    return new DictVal {
        {
            "uniform",
            new LambdaVal(new std::string[3]{"a", "b", ""},
                new ImplementExp(std_rand_uniform,NULL))
        }, {
            "normal",
            new LambdaVal(new std::string[3]{"a", "b", ""},
                new ImplementExp(std_rand_normal,NULL))
        }
    };
}

