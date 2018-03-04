#include "config.hpp"

bool werror = false;
bool WERROR() { return werror; }
void set_werror(bool b) { werror = b; }

bool verbosity = false;
bool VERBOSITY() { return verbosity; }
void set_verbosity(bool b) { verbosity = b; }

bool optimize = false;
bool OPTIMIZE() { return optimize; }
void set_optimize(bool b) { optimize = b; }

bool typesys = false;
bool USE_TYPES() { return typesys; }
void set_use_types(bool b) { typesys = b; }

