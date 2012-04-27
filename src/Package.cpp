#include "Package.h"

#include <fstream>

#include "Lexer.h"
#include "Parser.h"
#include "Generator.h"

#include "util.h"

using namespace std;

map<string, Package*> packages;

Package::Package(Generator *gen, string name) : Gen(gen), Ctx(this, gen) {
	Name = name;
	SymbolPrefix = "PIF_" + name + ".";
	PkgType = new PackageTypeAST(this);
	Complete = false;
	InitFunction = 0;
	Ctx.NamedValues.push_back(&Symbols);
}

bool Package::inputFile(string filename) {
	ifstream file(filename.c_str(), ios::in);

	if (file) {
		DBGB(cout << "Now parsing file : " << filename << endl)

		Lexer lex(filename, file);
		Parser parser(lex);
		while (1) {
			if (lex.tok == tok_eof) {
				break;
			} else if (lex.tok == tok_import) {
				ImportAST *i = parser.ParseImport();
				if (i == 0) return false;
				if (!import(i)) return false;
			} else if (lex.tok == tok_let || lex.tok == tok_var || lex.tok == tok_func) {
				DefAST *d;
				if (lex.tok == tok_func) {
					d = parser.ParseFuncDefinition();
				} else {
					d = parser.ParseVarDefinition();
				}
				if (d == 0) {
					return false;
				} else {
					DBGP(cerr << "def: " << d->Name << endl)

					if (Symbols.count(d->Name) == 0) {
						Symbol *s = new Symbol(d);
						s->SType = d->typeAtDef();
						Symbols[d->Name] = s;

						SymbolDefOrder.push_back(s);
					} else {
						std::cerr << d->Tag.str() << " Error: redefinition of symbol " << d->Name << endl;
						return false;
					}
				}
			} else {
				std::cerr << lex.tag().str() << " Error: expected definition in toplevel input." << endl;
				return false;
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
		Package *pkg = new Package(Gen, package_name);

		packages[package_name] = pkg;

		DBGB(cout << " - - - - importing " << package_name << " - - - - " << endl)

		vector<string> files;
		if (getdir(path + "/", files)) {
			cerr << "Package '" << package_name << "' not found." << endl;
			return false;
		}
		for (unsigned i = 0; i < files.size(); i++) {
			if (files[i].length() < 4) continue;
			if (files[i].substr(files[i].length()-4, 4) != ".pif") continue;
			string filename = path + "/" + files[i];
			if (!pkg->inputFile(filename)) {
				cerr << "Error somewhere in " << filename  << " or its deps, could not import '" << package_name << "'." << endl;
				return false;
			}
		}
		// Create dummy init function if needed
		if (pkg->Symbols.count("_init") == 0) {
			pkg->SymbolDefOrder.push_back( new Symbol(
				new FuncDefAST(
					FTag(),
					"_init",
					new FuncExprAST(
						FTag(),
						new FuncTypeAST(vector<FuncArgAST*>(), &voidBaseType),
						new BlockAST(FTag(), vector<StmtAST*>(1, 
							new ExprStmtAST(FTag(), new ReturnAST(FTag(), 0)))
						) 
					)
				)
			));
			pkg->Symbols["_init"] = pkg->SymbolDefOrder.back();
		}

		if (!pkg->typeCheck() || !Gen->build(pkg)) {
			cerr << "Error, could not import '" << package_name << "'." << endl;
			return false;
		}
		pkg->Complete = true;
		Gen->init(pkg);

		DBGB(cout << " - - - - imported " << package_name << " - - - - " << endl)

		Imports[def->As] = pkg;
	}
	return true;
}

// Check that everything is ok
bool Package::typeCheck() {
	bool checkVar = true;
	while (true) {
		for (unsigned i = 0; i < SymbolDefOrder.size(); i++) {
			if ((dynamic_cast<VarDefAST*>(SymbolDefOrder[i]->Def) != 0) != checkVar) continue;

			DBGP(cerr << "typecheck: " << SymbolDefOrder[i]->Def->Name << endl)

			if (!SymbolDefOrder[i]->Def->typeCheck(&Ctx)) {
				if (FuncDefAST* fd = dynamic_cast<FuncDefAST*>(SymbolDefOrder[i]->Def)) {
					fd->Val->prettyprint(cerr);
				}
				cerr << SymbolDefOrder[i]->Def->Tag.str() <<
					" Type error in definition of '" << SymbolDefOrder[i]->Def->Name << "'." << endl;
				return false;
			}
			SymbolDefOrder[i]->SType = SymbolDefOrder[i]->Def->typeAtDef();
		}

		if (checkVar) {
			checkVar = false;
		} else {
			return true;
		}
	}
}


// === Helper function for main ===
bool Package::importAndRunMain(string pkg) {
	vector<string> path;
	int j = 0;
	for (unsigned i = 0; i < pkg.length(); i++) {
		if (pkg[i] == '.') {
			path.push_back(pkg.substr(j, i-j));
			j = i + 1;
		}
	}
	path.push_back(pkg.substr(j, pkg.length()-j));
	string as = path.back();
	while (Imports.count(as) > 0) {
		as += "2";
	}
	if (!import(new ImportAST(FTag(), path, as))) return false;
	if (Imports.count(as) > 0) {
		return Gen->main(Imports[as]);
	} else {
		return false;
	}
}


