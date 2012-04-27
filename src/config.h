#ifndef DEF_CONFIG_H
#define DEF_CONFIG_H

#define FLOAT double
#define INT long long
#define INTSIZE (sizeof (INT) * 8) 

#define DEFAULT_PKG_PATH "packages"

/* Levels of verbosity for debugging informations :
	0 - no debug info
	1 - basic info : what packages & files we are parsing
	2 - process info : definition, typechecking and codegen of each function/symbol
	3 - content info : dump function code
	DEFAULT_DEBUG must be a string
*/
#define DEFAULT_DEBUG "3"


#endif

