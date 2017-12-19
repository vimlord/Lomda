#include "interp.hpp"
#include "config.hpp"

using namespace std;

#include <iostream>
#include <fstream>

void execute(string program) {
    Value *v = run(program);
    if (v) {
        cout << *v << "\n";
        delete v;
    }
}

void print_version() { cout << "Lambda 0.1.0\n"; }

/**
 * Runs the interpreted form of the program.
 *
 */
int interpret() {
    // Print relevant information
    print_version();
    cout << "enter a program and press <enter> to execute, or one of the following:\n";
    cout << "'exit' - exit the interpreter\n";

    string program;
    
    while (1) {
        // Get input
        cout << "> ";
        getline(cin, program);
        
        if (program == "exit")
            // Terminate the program
            return 0;
        else if (program != "") {
            // Evaluate the program
            execute(program);
        }
    }
}

int main(int argc, char *argv[]) {

    string filename = "";

    int i;
    for (i = 1; argv[i]; i++) {
        if (argv[i] == "--version") {
            print_version(); return 0;
        } else if (argv[i] == "--werror") {
            set_werror(true);
        } else {
            filename = argv[i];
            break;
        }
    }

    if (filename[0] == '\0')
        // Run the interpreter
        return interpret();
    else if (filename.length() < 5 || filename.substr(filename.length()-4) != ".lom") {
        // Files must be .lom files
        cerr << "\x1b[31m\x1b[1merror:\x1b[0m file '" << filename << "' does not have extension '.lom'\n";
    }

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

        if (i++) program += "\n";
        program += s;
    } while (file);
    
    // Parse and execute the program.
    execute(program);
}

