#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_

struct config {
    config() {}

    bool werror = false;
    bool verbosity = false;
    bool optimization = false;
    int  types = false;
    bool module_caching = false;

    char **argv = (char**) 0;
};

extern config configuration;

#endif
