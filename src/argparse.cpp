#include "argparse.hpp"

#include "config.hpp"
#include "main.hpp"
#include "tests.hpp"

#include <map>
#include <vector>
#include <iostream>

std::vector<cmdline_arg> args;

std::map<char, cmdline_arg> short_args;
std::map<std::string, cmdline_arg> long_args;

void add(cmdline_arg arg) {
    args.push_back(arg);
    if (arg.shortflag)
        short_args.insert({arg.shortflag, arg});
    if (arg.longflag != "")
        long_args.insert({arg.longflag, arg});
}

void cmdline_arg::operator()(char* arg) {
    if (handler)
        handler(arg);
}

void init_cmdline_args() {
    add(cmdline_arg("", 'c', [](char *prog) {
        execute(prog);
        exit(0);
    }, "Runs the given program directly from the command line.", true));
    add(cmdline_arg("help", 'h', [](char *a) {
        (void) a;

        // Display usage information
        std::cout << "usage: python [option] ... [-c cmd | file ]\noptions:\n";
        for (auto &it : args) {
            std::cout << "  ";

            // Argument name
            if (!it.shortflag)
                std::cout << "--" << it.longflag;
            else if (it.longflag == "")
                std::cout << "-" << it.shortflag;
            else
                std::cout << "-" << it.shortflag << " --" << it.longflag;
            
            if (it.has_arg)
                std::cout << " arg";
            
            std::cout << "\n";
            if (it.desc != "")
                std::cout << "\t" + it.desc + "\n";

        }
        exit(0);
    }, "Displays this information."));
    add(cmdline_arg("optimize", 'O', [](char *a) {
        (void) a;
        configuration.optimization = true;
    }, "Enables optimizing preprocessing."));
    add(cmdline_arg("", 't', [](char *a) {
        (void) a;
        // Run the lang tests
        int n = test();
        exit(n);
    }, "Runs built-in unit tests."));
    add(cmdline_arg("use-module-caching", 0, [](char *a) {
        (void) a;
        configuration.module_caching = true;
    }, "Enables caching of imported modules."));
    add(cmdline_arg("use-types", 0, [](char *a) {
        (void) a;
        configuration.types = configuration.werror ? 2 : 1;
    }, "Enables the type system."));
    add(cmdline_arg("verbose", 0, [](char *a) {
        (void) a;
        configuration.verbosity = true;
    }, "Displays the version of the interpreter."));
    add(cmdline_arg("version", 'v', [](char *a) {
        (void) a;
        print_version();
        exit(0);
    }, "Displays the version of the interpreter."));
    add(cmdline_arg("werror", 0, [](char *a) {
        (void) a;
        configuration.werror = true;
        // If we treat warnings as errors, then we will require typing to pass
        if (configuration.types)
            configuration.types = 2;
    }, "Enables treating of warnings as errors (type errors are equivalent to warnings)."));
}

std::string parse_cmdline_args(char *argv[]) {
    std::string filename = "";
    
    int i;
    for (i = 1; argv[i]; ++i) {
        cmdline_arg arg;
        if (argv[i][0] == '-') {
            if (argv[i][1] == '-') {
                // Parse a long form argument
                std::string nm(&argv[i][2]);
                auto it = long_args.find(nm);
                if (it == long_args.end()) {
                    std::cerr << argv[0] << ": unexpected argument '" << nm << "'\n";
                    exit(1);
                } else
                    arg = it->second;
            } else {
                // Parse a short form argument
                auto it = short_args.find(argv[i][1]);
                if (it == short_args.end() || argv[i][2]) {
                    std::cerr << argv[0] << ": unexpected argument '" << &argv[i][1] << "'\n";
                    exit(1);
                } else
                    arg = it->second;
            }
        } else {
            // A filename has been found.
            filename = argv[i++];
            break;
        }
        
        // Grab an argument on request
        char *x = arg.has_arg ? argv[++i] : NULL;

        // Perform processing
        arg(x);
    }

    return filename;
}


