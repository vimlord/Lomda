#ifndef _ARGPARSE_HPP_
#define _ARGPARSE_HPP_

#include <string>

struct cmdline_arg {
    // Each commandline argument has a short form and a long form
    char shortflag;
    std::string longflag;

    // There is also a function to be executed on parse
    void (*handler)(char*);

    // Args come with a description
    std::string desc;
    
    // Whether or not to pass an argument
    bool has_arg;


    cmdline_arg(std::string l = "", char s = 0, void (*h)(char*) = NULL, std::string d = "", bool arg=false)
    : shortflag(s), longflag(l), handler(h), desc(d), has_arg(arg) {}
    
    void operator()(char* = NULL);
};

void init_cmdline_args();
std::string parse_cmdline_args(char* argv[]);

#endif
