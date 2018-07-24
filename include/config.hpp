#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_

/**
 * Defines a configuration object to be used by the interpreter.
 */
struct config {
    config() {}
    
    // Whether or not warnings are to be treated as errors. Warnings are 
    bool werror = false;

    // Whether or not debug statements are to be printed.
    bool verbosity = false;

    // Whether or not optimizations will be provided.
    bool optimization = false;

    // Whether or not the type system will be used.
    int  types = false;

    // Whether or not module caching will be used.
    bool module_caching = false;

    // The arguments given to the program at runtime.
    char **argv = (char**) 0;
};

// The interpreter's configuration.
extern config configuration;

#endif
