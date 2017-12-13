#include "tests.hpp"

#include "expression.hpp"
#include "environment.hpp"
#include "value.hpp"

#include "bnf.hpp"

#include <iostream>
using namespace std;

int testcase(Expression *exp, Environment *env, string expect) {
    
    cout << "Case:\n" << *exp << "\nSubject to init env: " << *env << "\n";
    
    Value *v = exp->valueOf(env);
    string res = v ? v->toString() : "NULL";
    
    cout << " got: ";

    if (res == expect)
        cout << "\x1b[32m" << res;
    else
        cout << "\x1b[31m" << res << " [expected: " << expect << "]";

    cout << "\x1b[0m\n";
    
    delete exp;
    
    env->destroy(-1);
    delete env;

    delete v;

    cout << "\n";
    
    return res == expect ? 0 : 1;
}

int testexp1() {
    Expression *exp;
    Environment *env;
    
    exp = new SumExp(
        new IntExp(4),
        new SumExp(
            new IntExp(1),
            new IntExp(2))
    );

    env = new EmptyEnv();
    
    return testcase(exp, env, IntVal(7).toString());
}

int testexp2() {
    Expression *exp;
    Environment *env; 
    
    string vs[2];
    vs[0] = "f";
    vs[1] = "";

    string *xs = new string[4]();
    xs[0] = "x";
    xs[1] = "";

    Expression *zs[2];
    zs[0] = new LambdaExp(xs, 
        new SumExp(
            new VarExp("x"), 
            new IntExp(2)
        ));
    zs[1] = NULL;

    Expression *args[4];
    args[0] = new IntExp(1);
    args[1] = NULL;

    exp = new LetExp(
        // let f = lambda x -> x+2
        vs, zs,
        // in f(1)
        new ApplyExp(new VarExp("f"), args)
    );
    env = new EmptyEnv();

    return testcase(exp, env, IntVal(3).toString());
}

int testexp3() {
    Expression *exp;
    Environment *env; 
    
    string *xs = new string[2]();
    Expression **zs = new Expression*[2]();
    xs[0] = "x"; xs[1] = "";
    zs[0] = new IntExp(1);
    // f(x) = 3*x + 1
    Expression *f = new LambdaExp(xs, 
        new SumExp(
            new MultExp(new IntExp(3), new VarExp("x")),
            new IntExp(1)));

    exp = new ApplyExp(
            new DerivativeExp(f, "x"), // df/dx
            zs
    );
    
    env = new EmptyEnv();

    return testcase(exp, env, IntVal(3).toString());
}

int testexp4() {
    Expression *exp;
    Environment *env;

    string *as = new string[2];
    Expression **bs = new Expression*[2];

    as[0] = "x"; as[1] = "";
    bs[0] = new IntExp(1); bs[1] = NULL;

    Expression **xs = new Expression*[2];
    Expression **zs = new Expression*[2];

    xs[0] = new VarExp("x"), xs[1] = NULL;
    zs[0] = new IntExp(2); zs[1] = NULL;

    exp = 
        new LetExp(as, bs,
        new SequenceExp(
        new SetExp(xs, zs),
        new VarExp("x")
    ));
    env = new EmptyEnv();

    return testcase(exp, env, IntVal(2).toString());

}

int testtype_exp() {
    cout << "\x1b[4m" << "Running expression test\n" << "\x1b[0m";

    int fails =
        testexp1()
        + testexp2()
        + testexp3()
        + testexp4()
    ;

    return fails;
}


int testtype_env() {
    int fails = 0;

    string type;

    cout << "\x1b[4m" << "Running environment test\n" << "\x1b[0m";

    Environment* env = new EmptyEnv();

    if (env->apply("x") != NULL) {
        std::cout << " EmptyEnv.apply() failed to return NULL\n";
        fails++;
    }
    
    cout << "Case: (empty-env) <- 3...\n";
    Value *val = new IntVal(3);
    env = new ExtendEnv("x", val, env);

    if (typeid(*env) != typeid(ExtendEnv)) {
        std::cout << "\x1b[31mEmptyEnv.insert() failed to convert to ExtendEnv\x1b[0m\n";
        cout << " got: \x1b[31m" << *env << "\n";
        fails++;
    } else
        cout << " got: \x1b[32m" << *env << "\n";
    cout << "\x1b[0m";

    if (env->apply("x") != val) {
        std::cout << " \x1b[31mExtendEnv.apply() failed to retrieve the inserted value\x1b[0m\n";
        fails++;
    }

    cout << "Case: env.subenvironment...\n";

    env = env->subenvironment();
    if (typeid(*env) != typeid(EmptyEnv)) {
        std::cout << " \x1b[31mExtendEnv.remove() failed to convert to EmptyEnv\x1b[0m\n";
        cout << " got: \x1b[31m" << *env << "\n";
        fails++;
    } else
        cout << " got: \x1b[32m" << *env << "\n";
    cout << "\x1b[0m";
    
    cout << "\n";
    return fails;
}

int test_parsecase(string program, string out) {
    int score = 1;
    std::cout << "Case: " << program << "\n";

    List<Expression*> *ps = parse(program);
    
    if (ps->size() == 0) {
        std::cout << " \x1b[31mno match found\n";
    } else if (ps->size() == 1) {
        Expression *exp = ps->get(0);
        
        std::cout << " got: ";
        std::cout << (exp->toString() == out ? "\x1b[32m" : "\x1b[31m");
        std::cout << *exp << "\n";

        score = 0;
    } else {
        std::cout << " \x1b[31mambiguous result:\n";
        Iterator<int, Expression*> *it = ps->iterator();
        while (it->hasNext())
            std::cout << "  " << *(it->next()) << "\n";
    }

    std::cout << "\x1b[0m\n";

    return score;
}

int test_parse() {
    cout << "\x1b[4m" << "Running parser test\n" << "\x1b[0m";

    int fails = 0
      + test_parsecase("let x = 2; 1", "let x = 2; 1")
      + test_parsecase("x + 3", "x + 3")
      + test_parsecase("let f = lambda (x) x; f(2)", "let f = lambda (x) x; f(2)")
    ;

    return fails;
}


int test() {
    int n = 0;

    n += testtype_env();
    n += testtype_exp();
    n += test_parse();

    cout << "\x1b[1m";
    
    if (n)
        cout << "\x1b[31m" << "Failed " << n << " test cases\n";
    else
        cout << "\x1b[32m" << "All test cases passed!\n";

    cout << "\x1b[0m";
    
    return n ? 1 : 0;

}


