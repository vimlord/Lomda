#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_

// Werror operates the same as in gcc; all warnings become errors
bool WERROR();
void set_werror(bool b);

bool VERBOSITY();
void set_verbosity(bool b);

bool OPTIMIZE();
void set_optimize(bool b);

#endif
