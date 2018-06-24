#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_

// Werror operates the same as in gcc; all warnings become errors
bool WERROR();
void set_werror(bool b);

bool VERBOSITY();
void set_verbosity(bool b);

bool OPTIMIZE();
void set_optimize(bool b);

int USE_TYPES();
void set_use_types(int b);

bool USE_MODULE_CACHING();
void set_use_module_caching(bool b);

char** get_argv();
void set_argv(char **argv);

#endif
