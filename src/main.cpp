#include "interp.hpp"
#include "expression.hpp"
#include "config.hpp"

#include "tests.hpp"

using namespace std;

#include <iostream>
#include <fstream>

#include <cstddef>
#include <cstring>

#include <readline/readline.h>
#include <readline/history.h>

void execute(string program) {
    Val v = run(program);
    v = unpack_thunk(v);

    if (v) {
        if (VERBOSITY()) cout << "(" << v << ")\n==> ";
        cout << *v << "\n";
        v->rem_ref();
    }
}

void print_version() {
    // Version number
    cout << "Lomda v0.1.0\n";

    // Compilation time
    cout << "Compiled " << __DATE__ << " @ " << __TIME__ << "\n";
}

void display_config() {
    std::cout << "The following configuration is in use:\n";
    std::cout << "mod cache: " << OPTIMIZE() << " (default: 0)\n";
    std::cout << "optimize:  " << OPTIMIZE() << " (default: 0)\n";
    std::cout << "use_types: " << USE_TYPES() << " (default: 0)\n";
    std::cout << "verbosity: " << VERBOSITY() << " (default: 0)\n";
    std::cout << "werror:    " << WERROR() << " (default: 0)\n";
}

/**
 * Runs the interpreted form of the program.
 *
 */
int interpret() {
    // Print relevant information (only print the config data if in verbose mode)
    print_version();
    if (VERBOSITY()) display_config();

    cout << "Enter a program and press <enter> to execute, or one of the following:\n";
    cout << "'exit' - exit the interpreter\n";
    cout << "'q/quit' - exit the interpreter\n";

    char *buff;
    
    while ((buff = readline("> "))) {
        // Use a string to hold it
        string program(buff);
        
        // If it was not a blank string, hold onto it
        if (*buff) add_history(buff);

        // Garbage collection has to be done manually.
        free(buff);

        if (program == "exit" || program == "quit" || program == "q")
            // Terminate the program
            return 0;
        else if (program != "") {
            // Evaluate the program
            execute(program);
        }
    }

    return 0;
}

int main(int argc, char *argv[]) { 
    set_argv(argv);

    string filename = "";

    int i;
    for (i = 1; argv[i]; i++) {
        if (!strcmp(argv[i], "--use-types"))
            set_use_types(WERROR() ? 2 : 1);
        else if (!strcmp(argv[i], "--use-module-caching"))
            set_use_module_caching(true);
        else if (!strcmp(argv[i], "--version")) {
            print_version(); return 0;
        } else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) {
            set_verbosity(true);
        } else if (!strcmp(argv[i], "--werror")) {
            set_werror(true);
            // If we treat warnings as errors, then we will require typing to pass
            if (USE_TYPES())
                set_use_types(2);
        } else if (!strcmp(argv[i], "-O") || !strcmp(argv[i], "--optimize")) {
            set_optimize(true);
        } else if (!strcmp(argv[i], "-t")) {
            // Run the lang tests
            int n = test();
            ImportExp::clear_cache();
            return n;
        } else {
            filename = argv[i];
            break;
        }
    }

    if (filename[0] == '\0')
        // Run the interpreter
        interpret();
    else if (filename.length() < 5 || filename.substr(filename.length()-4) != ".lom") {
        // Files must be .lom files
        cerr << "\x1b[31m\x1b[1merror:\x1b[0m file '" << filename << "' does not have extension '.lom'\n";
        return 1;
    } else {
        // Attempt to open the file
        ifstream file;
        file.open(filename);
        
        // Ensure that the file exists
        if (!file) {
            cerr << "\x1b[31m\x1b[1merror:\x1b[0m could not load program from '" << argv[1] << "'\n";
            return 1;
        }
        
        // Read the program from the input file
        i = 0;
        string program = "";
        do {
            string s;
            getline(file, s);

            if (i++) program += " ";
            program += s;
        } while (file);

        throw_debug("IO", "program: '" + program + "'\n");
        file.close();

        display_config();
        
        // Parse and execute the program.
        execute(program);
    }
    
    // Clear the cache
    ImportExp::clear_cache();
    
    return 0;
}

