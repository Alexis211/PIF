#ifndef DEF_UTIL_H
#define DEF_UTIL_H

#include <string>
#include <vector>

extern std::string pkgPath;

extern bool DEBUG;

int getdir(std::string dir, std::vector<std::string> &files);

#endif

