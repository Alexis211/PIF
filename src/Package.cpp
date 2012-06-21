#include "Package.h"

#include <fstream>

#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "codegen-llvm/Generator.h"

#include "util.h"
#include "error.h"

using namespace std;

map<string, Package*> packages;

// Accessors

Package *Package::getImport(string name) {
	if (Imports.count(name) > 0) return Imports[name];
	return 0;
}

//Stuff

Package::Package(Generator *gen, string name) : Gen(gen), Ctx(this, gen) {
	Name = name;
	SymbolPrefix = "PIF_" + name + ".";
	PkgType = new PackageTypeAST(this);
	Complete = false;
	InitFunction = 0;
	Ctx.NamedValues.push_back(&Symbols);
}

void Package::inputFile(string filename) {
	ifstream file(filename.c_str(), ios::in);

	if (file) {
		DBGB(cout << " - parsing " << filename << endl)

		Lexer lex(filename, file);
		Parser parser(lex);
		while (1) {
			if (lex.tok == tok_eof) {
				break;
			} else if (lex.tok == tok_import) {
				ImportAST *i = parser.ParseImport();
				import(i);
			} else if (lex.tok == tok_let || lex.tok == tok_var || lex.tok == tok_func) {
				DefAST *d;
				if (lex.tok == tok_func) {
					d = parser.ParseFuncDefinition();
				} else {
					d = parser.ParseVarDefinition();
				}
				DBGP(cerr << "def: " << d->Name << endl)

				if (Symbols.count(d->Name) == 0) {
					Symbol *s = new Symbol(d);
					s->SType = d->typeAtDef();
					Symbols[d->Name] = s;

					SymbolDefOrder.push_back(s);
				} else {
					d->Tag.Throw("Error: redefinition of symbol " + d->Name);
				}
			} else {
				lex.tag().Throw("Error: expected definition in toplevel input.");
			}
		}
	} else {
		throw new PIFError("Unable to open file '" + filename + "'.");
	}
}

void Package::import(ImportAST *def) {
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
			throw new PIFError("Cannot import '" + package_name + "': dependency cycle or error in package.");
		}
	} else {
		Package *pkg = new Package(Gen, package_name);

		packages[package_name] = pkg;

		DBGB(cout << " -> Importing " << package_name << endl)

		vector<string> files;
		if (getdir(path + "/", files)) {
			throw new PIFError("Package '" + package_name + "' not found.");
		}
		for (unsigned i = 0; i < files.size(); i++) {
			if (files[i].length() < 4) continue;
			if (files[i].substr(files[i].length()-4, 4) != ".pif") continue;
			string filename = path + "/" + files[i];
			try {
				pkg->inputFile(filename);
			} catch (PIFError *e) {
				throw new PIFError(
					"Error somewhere in " + filename  +
					" or its deps, could not import '" + package_name + "'.", e);
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
						FuncTypeAST::Get(vector<FuncArgAST*>(), BaseTypeAST::Get(bt_void)),
						new BlockAST(FTag(), vector<StmtAST*>(1, 
							new ExprStmtAST(FTag(), new ReturnAST(FTag(), 0)))
						) 
					)
				)
			));
			pkg->Symbols["_init"] = pkg->SymbolDefOrder.back();
		}

		try {
			pkg->typeCheck();
			Gen->build(pkg);
		} catch (PIFError *e) {
			throw new PIFError("In importing package '" + package_name + "'.", e);
		}
		pkg->Complete = true;
		Gen->init(pkg);

		DBGB(cout << " <- Imported " << package_name << endl)

		Imports[def->As] = pkg;
	}
}

// Check that everything is ok
void Package::typeCheck() {
	bool checkVar = true;		// first check variable definitions, then functions
	while (true) {
		for (unsigned i = 0; i < SymbolDefOrder.size(); i++) {
			if ((dynamic_cast<VarDefAST*>(SymbolDefOrder[i]->Def) != 0) != checkVar) continue;

			DBGP(cerr << "typecheck: " << SymbolDefOrder[i]->Def->Name << endl)

			try {
				SymbolDefOrder[i]->Def->typeCheck(&Ctx);
			} catch (PIFError *e) {
				if (FuncDefAST* fd = dynamic_cast<FuncDefAST*>(SymbolDefOrder[i]->Def)) {
					fd->Val->prettyprint(cerr);
				}
				SymbolDefOrder[i]->Def->Tag.Throw(
					"Type error in definition of '" + SymbolDefOrder[i]->Def->Name + "'.", e);
			}
			SymbolDefOrder[i]->SType = SymbolDefOrder[i]->Def->typeAtDef();
		}

		if (checkVar) {
			checkVar = false;
		} else {
			return;
		}
	}
}


// === Helper function for main ===
void Package::importAndRunMain(string pkg) {
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
	import(new ImportAST(FTag(), path, as));

	if (Imports.count(as) > 0) {
		Gen->main(Imports[as]);
	}
}


