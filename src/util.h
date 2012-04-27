#ifndef DEF_UTIL_H
#define DEF_UTIL_H

#include <string>
#include <set>
#include <map>
#include <vector>

extern std::string pkgPath;

extern int DEBUGLevel;

// See config.h for more info
#define DBG(L, E) if (DEBUGLevel >= L) { E; }
#define DBGB(E) if (DEBUGLevel >= 1) { E; }
#define DBGP(E) if (DEBUGLevel >= 2) { E; }
#define DBGC(E) if (DEBUGLevel >= 3) { E; }

int getdir(std::string dir, std::vector<std::string> &files);

class ArgParser {
	private:
	std::string BinName;
	std::set<std::string> BoolOpts;
	std::map<std::string, std::string> StrOpts;
	std::vector<std::string> Params;

	public:
	ArgParser(int argc, char *argv[]);

	void addBool(std::string str);
	void addStr(std::string str, std::string deflt = "");

	std::string getBinName();
	bool getBool(std::string str);
	std::string getStr(std::string str);
	const std::vector<std::string> &getParams();

};

#endif

