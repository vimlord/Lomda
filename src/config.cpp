#include "config.hpp"

bool werror = false;
bool WERROR() { return werror; }
void set_werror(bool b) { werror = b; }

bool verbosity = false;
bool VERBOSITY() { return verbosity; }
void set_verbosity(bool b) { verbosity = b; }

