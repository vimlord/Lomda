#include "interp.hpp"

using namespace std;

#include <iostream>

int main(int argc, char *argv[]) {
    string program;

    while (1) {
        cout << "> ";
        getline(cin, program);
        
        if (program == "exit")
            return 0;
        else if (program != "") {
            Value *v = run(program);
            if (v)
                cout << *v << "\n";
        }
    }
}

