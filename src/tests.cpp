#include "tests.hpp"

#include "bnf.hpp"
#include "environment.hpp"
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
    
    if (!file) {
        throw_err("", "could not load test cases from '" + fname + "'");
    } else for (int i = 1; file; i++) {
        string x, y;

        // Get the input line
        do { getline(file, x); } while (file && (x.length() == 0 || x[0] == '#'));
        
        if (!file) break;

        // Get the output line
        getline(file, y);
        
        Exp exp = compile(x);

        if (exp) {
            // Verify the legality of the case
            Trie<bool> *vardta = new Trie<bool>;
            bool valid = exp->postprocessor(vardta);
            delete vardta;

            // Keep the case if it passes the postprocessor
            if (valid)
                cases[exp] = y;
            else
                std::cout << "Test case " << i << " from '" << fname << "' will not be loaded (failed postprocessor)\n";
        } else
            // Discard it.
            std::cout << "Test case " << i << " from '" << fname << "' will not be loaded (failed BNF parsing)\n";
    }

    file.close();
    std::cout << "Loaded " << cases.size() << " test cases from '" << fname << "'\n";

    return cases;
}

int test_interp_cases(string title) {
    int n = 0;
    int i = 1;
    auto interp_cases = load_test_cases("./tests/" + title + ".cases");

    for (auto it : interp_cases) {
        // Evaluate the test case
        Exp exp = it.first;
        Env env = new EmptyEnv;
        Val v = exp->evaluate(env);
        
        // Garbage collection
        delete exp;
        env->rem_ref();

        if (!v || v->toString() != it.second) {
            std::cout << "Failed test case " << title << ":" << to_string(i) << "\n";
            std::cout << "\tExpected " << it.second << ", got " << (v ? v->toString() : "null") << "\n";
            n++;
        }

        // Destroy the value
        v->rem_ref();

        // Counter
        i++;
    }

    return n;
}

int test() {
    int n = 0;

    n += test_interp_cases("interp");

    // Display end results
    std::cout << "\n";
    if (n)
        cout << "\x1b[31m" << "Failed " << n << " test cases\n";
    else
        cout << "\x1b[32m" << "All test cases passed!\n";

    cout << "\x1b[0m";
    
    return n ? 1 : 0;

}


