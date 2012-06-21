#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <stdlib.h>

#include <llvm/Support/raw_ostream.h>

#include "codegen-llvm/Generator.h"

#include "util.h"
#include "error.h"

using namespace std;

string pkgPath = DEFAULT_PKG_PATH;
int DEBUGLevel;

extern "C" void print_int(INT i) {
	cout << i << " " << flush;
}

extern "C" void print_float(FLOAT f) {
	cout << f << " " << flush;
}

extern "C" void print_nl() {
	cout << endl;
}

int main(int argc, char *argv[]) {
	cout << endl << "PIF compiler version 0.1 - adnab.fr.nf 2012" << endl << endl;

	ArgParser args(argc, argv);
	args.addStr("-d", DEFAULT_DEBUG);

	DEBUGLevel = atoi(args.getStr("-d").c_str());

	Generator *gen = new Generator();
	Package *pkg = new Package(gen, "_");		// Interpreter context

	const vector<string> &pkgs = args.getParams();
	if (pkgs.size() == 0) {
		cout << "Usage : " << args.getBinName() << " [options] package..." << endl;
		cout << "Options:" << endl;
		cout << "    -d <debug_level>\tVerbosity level for debug information (default: " << DEFAULT_DEBUG << ")" << endl;
		cout << "\t\t\tSee source in config.h for detailed info about debug level." << endl;
		cout << endl;
	} else {
		try {
			for (unsigned i = 0; i < pkgs.size(); i++) {
				pkg->importAndRunMain(pkgs[i]);
			}
		} catch (PIFError *e) {
			e->disp();
			cerr << "KYAAAA ! IT DIDN'T COMPILE !!" << endl;
		}
	}

	std::string err;
	llvm::raw_fd_ostream out("dump.llvm", err, 0);
	gen->TheModule->print(out, 0);

	return 0;
}

