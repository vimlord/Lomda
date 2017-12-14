#include "interp.hpp"

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

int interpret() {
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
    if (argc == 1)
        // Run the interpreter
        return interpret();
    else if (argc == 2) {
        // Attempt to open the file
        ifstream file;
        file.open(argv[1]);
        
        // Ensure that the file exists
        if (!file) {
            cerr << "\x1b[31m\x1b[1merror:\x1b[0m could not load program from '" << argv[1] << "'\n";
            return 1;
        }
        
        // Read the program from the input file
        int i = 0;
        string program = "";
        do {
            string s;
            getline(file, s);

            if (i++) program += "\n";
            program += s;
        } while (file);
        
        // Parse and execute the program.
        execute(program);

    } else {
        // Throw an error
        cerr << "usage: " << argv[0] << " [file | ]\n";
        return 1;
    }
}

