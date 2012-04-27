#include "Package.h"

#include <fstream>

#include "Lexer.h"
#include "Parser.h"
#include "Generator.h"

#include "util.h"

using namespace std;

map<string, Package*> packages;

Package::Package(Generator *gen) {
	Gen = gen;
	Complete = false;
}

bool Package::inputFile(string filename) {
	ifstream file(filename.c_str(), ios::in);

	if (file) {
		cout << "Now parsing file : " << filename << endl;
		Lexer lex(filename, file);
		Parser parser(lex);
		while (1) {
			if (lex.tok == tok_eof) {
				break;
			} else if (lex.tok == tok_import) {
				ImportAST *i = parser.ParseImport();
				if (i == 0) return false;
				if (!import(i)) return false;
			} else if (lex.tok == tok_let) {
				DefAST *d = parser.ParseDefinition();
				if (d == 0) {
					return false;
				} else {
					if (Symbols.count(d->Name) == 0) {
						Symbols[d->Name] = d;
						if ((!dynamic_cast<FuncExprAST*>(d->Val)) && (!dynamic_cast<ExternAST*>(d->Val))) {
							InitCode.push_back(new BinaryExprAST(d->Tag, "=", new VarExprAST(d->Tag, d->Name), d->Val));
						}
					} else {
						std::cerr << d->Tag.str() << " Error: redefinition of symbol " << d->Name << endl;
						return false;
					}
				}
			} else {
				ExprAST *e = parser.ParseExpression();
				if (e == 0) {
					return false;
				} else {
					InitCode.push_back(e);
				}
			}
		}
	} else {
		cerr << "Unable to open file '" << filename << "'." << endl;
		return false;
	}
	return true;
}

bool Package::import(ImportAST *def) {
	string path = pkgPath;
	string package_name = "";
	for (unsigned int i = 0; i < def->Path.size(); i++) {
		path += "/" + def->Path[i];
		if (i != 0) package_name += ".";
		package_name += def->Path[i];
	}

	if (packages.count(package_name)) {
		Package *pkg = packages[package_name];
		if (pkg->Complete) {
			Imports[def->As] = pkg;
		} else {
			cerr << "Cannot import '" << package_name << "': dependency cycle or error in package." << endl;
			return false;
		}
	} else {
		Package *pkg = new Package(Gen);

		pkg->SymbolPrefix = "PIF_";
		for (unsigned int i = 0; i < def->Path.size(); i++) {
			pkg->SymbolPrefix += "_" + def->Path[i];
		}
		pkg->SymbolPrefix += "__";

		packages[package_name] = pkg;
		cout << " - - - - importing " << package_name << " - - - - " << endl;
		//TODO: import all files
		{
			std::string filename = path + ".pif";
			if (!pkg->inputFile(filename)) {
				cerr << "Error in " << filename  << ", could not import '" << package_name << "'." << endl;
				return false;
			}
		}
		if (!Gen->build(pkg)) {
			cerr << "Error, could not import '" << package_name << "'." << endl;
			return false;
		}
		pkg->Complete = true;
		Gen->init(pkg);
		cout << " - - - - finished importing " << package_name << " - - - - " << endl;
		Imports[def->As] = pkg;
	}
	return true;
}
