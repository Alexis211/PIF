#ifndef DEF_PACKAGE_H
#define DEF_PACKAGE_H

#include <map>
#include <string>

#include "config.h"

#include "AST.h"

class Generator;

class Package {
	friend class Generator;
	friend class VarExprAST;
	private:
	Generator *Gen;

	std::map<std::string, Package*> Imports;
	std::map<std::string, TypeAST*> Types;		//TODO: will be implemented later
	std::map<std::string, DefAST*> Symbols;

	std::vector<ExprAST*> InitCode;
	llvm::Function *InitFunction;

	std::string SymbolPrefix;

	bool Complete;

	public:

	Package(Generator *gen);

	bool inputFile(std::string filename);

	bool import(ImportAST *def);
};

class Context {
	public:
	Package *Pkg;
	Generator *Gen;
	llvm::Function *Fun;

	std::map<std::string, llvm::Value*> NamedValues;

	Context(Package *p, Generator *g) : Pkg(p), Gen(g), Fun(0) {}
};

// extern std::map<std::string, Package*> Packages;

#endif

