#ifndef DEF_PACKAGE_H
#define DEF_PACKAGE_H

#include <map>
#include <string>
#include <iostream>

#include "config.h"

#include "ast/stmt.h"
#include "ast/expr.h"

class Generator;
class Package;

class Symbol {
	public:
	DefAST *Def;
	TypeAST *SType;
	llvm::Value *llvmVal;

	Symbol(DefAST *def) : Def(def) {
		llvmVal = 0;
		SType = 0;
	}
	Symbol(TypeAST *type, llvm::Value *v) {
		Def = 0;
		SType = type;
		llvmVal = v;
	}
};

class MoreContext {
	public:
	TypeAST *FuncRetType;
	llvm::BasicBlock *BreakTo;
	llvm::BasicBlock *ContinueTo;

	MoreContext(TypeAST *ret) : FuncRetType(ret), BreakTo(0), ContinueTo(0) {}
};

class Context {
	public:
	Package *Pkg;
	Generator *Gen;
	MoreContext *More;

	std::vector< std::map<std::string, Symbol*>* > NamedValues;

	Context(Package *p, Generator *g) : Pkg(p), Gen(g), More(0) {}
};

class Package {
	friend class Generator;
	friend class DotMemberExprAST;
	friend class PackageTypeAST;
	friend int main(int argc, char *argv[]);

	private:
	PackageTypeAST *PkgType;

	Generator *Gen;
	Context Ctx;

	std::map<std::string, Package*> Imports;
	std::map<std::string, TypeAST*> Types;		//TODO: will be implemented later
	std::map<std::string, Symbol*> Symbols;
	std::vector<Symbol*> SymbolDefOrder;

	llvm::Function *InitFunction;

	std::string Name;
	std::string SymbolPrefix;

	bool Complete;

	public:

	Package(Generator *gen, std::string name);

	void inputFile(std::string filename);
	void typeCheck();

	void import(ImportAST *def);

	void importAndRunMain(std::string pkg);

	Package *getImport(std::string name);
};

// extern std::map<std::string, Package*> Packages;

#endif

