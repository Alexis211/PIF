#include <iostream>
#include <string>
#include <map>
#include <vector>

#include <llvm/Support/raw_ostream.h>

#include "Generator.h"

using namespace std;

string pkgPath = DEFAULT_PKG_PATH;
bool DEBUG = DEFAULT_DEBUG;

extern "C" void print_int(INT i) {
	cout << i << " " << flush;
}

extern "C" void print_float(FLOAT f) {
	cout << f << " " << flush;
}

extern "C" void print_nl() {
	cout << endl;
}

int main() {
	cout << "PIF compiler version 0.1 - adnab.fr.nf 2012\n" << endl;

	Generator *gen = new Generator();

	Package *pkg = new Package(gen, "__pif_main");		// Interpreter context
	vector<string> v; v.push_back("test");
	pkg->import(new ImportAST(FTag(), v, "main"));
	if (pkg->Imports.count("main") > 0) {
		gen->main(pkg->Imports["main"]);
	} else {
		cerr << "KYAAAA ! IT DIDN'T COMPILE !!" << endl;
	}

	std::string err;
	llvm::raw_fd_ostream out("dump.llvm", err, 0);
	gen->TheModule->print(out, 0);

	return 0;
}

