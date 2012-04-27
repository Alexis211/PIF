#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <algorithm>

#include "util.h"

using namespace std;

// Thank the internet for letting me steal this code.
int getdir (string dir, vector<string> &files) {
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cerr << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL) {
        files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}


// ARGUMENT PARSER !

ArgParser::ArgParser(int argc, char *argv[]) : BinName(argv[0]) {
	for (int i = 1; i < argc; i++) {
		Params.push_back(string(argv[i]));
	}
}

void ArgParser::addBool(string str) {
	auto itr = find(Params.begin(), Params.end(), str);
	if (itr != Params.end()) {
		Params.erase(itr);
		BoolOpts.insert(str);
	}
}

void ArgParser::addStr(string str, string deflt) {
	StrOpts[str] = deflt;
	auto itr = find(Params.begin(), Params.end(), str);
	if (itr != Params.end()) {
		auto itr2 = itr;
		itr2++;
		if (itr2 != Params.end()) {
			StrOpts[str] = *itr2;
			itr2++;
			Params.erase(itr, itr2);
		}
	}
}

string ArgParser::getBinName() {
	return BinName;
}

bool ArgParser::getBool(string str) {
	return (BoolOpts.count(str) > 0);
}

string ArgParser::getStr(string str) {
	return StrOpts[str];
}

const std::vector<std::string> &ArgParser::getParams() {
	return Params;
}

