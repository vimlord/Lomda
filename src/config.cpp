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

int typesys = 0;
int USE_TYPES() { return typesys; }
void set_use_types(int b) { typesys = b; }


bool modcache = 0;
bool USE_MODULE_CACHING() { return modcache; }
void set_use_module_caching(bool b) { modcache = b; }

