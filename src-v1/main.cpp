#include <iostream>
#include <string>
#include <map>
#include <vector>

#include <llvm/Support/raw_ostream.h>

#include "Generator.h"

using namespace std;

string pkgPath = "packages";

extern "C" void print_int(INT i) {
	cout << i << endl;
}

extern "C" void print_float(FLOAT f) {
	cout << f << endl;
}

int main() {
	cout << "PIF compiler version 0.1 - adnab.fr.nf 2012\n" << endl;

	Generator *gen = new Generator();

	Package *pkg = new Package(gen);		// Interpreter context
	vector<string> v; v.push_back("test");
	pkg->import(new ImportAST(FTag("_", 0), v, "math"));

	std::string err;
	llvm::raw_fd_ostream out("dump.llvm", err, 0);
	gen->TheModule->print(out, 0);

	return 0;
}

