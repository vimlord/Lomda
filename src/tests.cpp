#include "tests.hpp"

#include "parser.hpp"

#include "baselang/environment.hpp"
#include "interp.hpp"

#include <iostream>
#include <fstream>

#include <cstddef>
#include <cstring>

#include <map>
#include <vector>

using namespace std;

#include <unistd.h>


inline string txt_bold(string x) { return "\x1b[1m" + x + "\x1b[0m"; }

/**
 * Given a file, load a set of test cases
 */
std::map<Exp,string> load_test_cases(string fname) {
    map<Exp,string> cases;

    ifstream file;
    file.open(fname);

    int failures = 0;
    
    if (!file) {
        throw_err("", "could not load test cases from '" + fname + "'");
    } else for (int i = 1; file; i++) {
        string x, y;

        // Get the input line
        do { getline(file, x); } while (file && (x.length() == 0 || x[0] == '#'));
        
        if (!file) break;

        // Get the output line
        getline(file, y);
        
        Exp exp = parse_program(x);

        if (exp) {
            // Verify the legality of the case
            Trie<bool> *vardta = new Trie<bool>;
            bool valid = exp->postprocessor(vardta);
            delete vardta;

            // Keep the case if it passes the postprocessor
            if (valid)
                cases[exp] = y;
            else {
                delete exp;
                std::cout << "Test case " << i << " from '" << fname << "' will not be loaded (failed postprocessor)\n";
                failures++;
            }
        } else {
            // Discard it.
            std::cout << "Test case " << i << " from '" << fname << "' will not be loaded (failed BNF parsing)\n";
            std::cout << "Case: " << x << "\n";
            failures++;
        }
    }

    file.close();
    std::cout << "Loaded " << cases.size() << " test cases from '" << fname << "'\n";

    // Store the number of failing test cases
    cases[NULL] = to_string(failures);

    return cases;
}

string run_interp(Exp x) {
    Env env = new Environment;
    Val v = x->evaluate(env);
    if (!v) return "NULL";

    env->rem_ref();
    
    string res = v->toString();
    v->rem_ref();

    return res;
}

string run_types(Exp x) {
    Tenv tenv = new TypeEnv;
    Type *t = x->typeOf(tenv);
    if (!t) return "NULL";
    
    delete tenv;
    
    string res = t->toString();
    delete t;

    return res;
}

int test_cases(string title, string (*run)(Exp)) {
    int n = 0;
    int i = 1;
    auto interp_cases = load_test_cases("./tests/" + title + ".cases");
    
    // The number of cases that do not parse
    n += stoi(interp_cases[NULL]);
    interp_cases.erase(NULL);

    for (auto it : interp_cases) {
        // Evaluate the test case
        Exp exp = it.first;

        if (VERBOSITY()) std::cout << "Testing " << *exp << "\n"; 

        string res = run(exp);
        
        // Garbage collection
        delete exp;

        if (res != it.second) {
            std::cout << "Failed test case " << title << ":" << to_string(i) << "\n";
            std::cout << "\tExpected " << it.second << ", got " << res << "\n";
            n++;
        }

        // Counter
        i++;
    }

    return n;
}

int test() {
    int n = 0;

    n += test_cases("interp", run_interp);
    n += test_cases("types", run_types);

    // Display end results
    std::cout << "\n";
    if (n)
        cout << "\x1b[31m" << "Failed " << n << " test cases\n";
    else
        cout << "\x1b[32m" << "All test cases passed!\n";

    cout << "\x1b[0m";
    
    return n ? 1 : 0;

}


