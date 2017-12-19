#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_

// Werror operates the same as in gcc; all warnings become errors
static bool werror = false;
static void set_werror(bool b) { werror = b; }

#endif
