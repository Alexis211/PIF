#ifndef DEF_AST_STMT_H
#define DEF_AST_STMT_H

#include "../config.h"
#include "../lexer/Lexer.h"

#include <string>
#include <map>
#include <vector>
#include <iostream>

#include "expr.h"

class StmtAST {
	friend class Package;

	public:
	StmtAST(const FTag &tag) : Tag(tag) {}
	virtual ~StmtAST() {}

	const FTag Tag;

	virtual void typeCheck(Context *ctx) {};
};

// ExprStmtAST - Class for statements made of expressions
class ExprStmtAST : public StmtAST {
	friend class BlockAST;
	
	private:
	ExprAST *Expr;

	public:
	ExprStmtAST(const FTag &tag, ExprAST *expr) : StmtAST(tag), Expr(expr) {}
	virtual ~ExprStmtAST() {}

	virtual void typeCheck(Context *ctx);
};

// ImportAST - Class for import statements
class ImportAST : public StmtAST {
	friend class Package;
	private:
	std::vector<std::string> Path;
	std::string As;

	public:
	virtual ~ImportAST() {}
	ImportAST(const FTag &tag, const std::vector<std::string> &path, const std::string &as) : 
		StmtAST(tag),  Path(path), As(as) {}
};

// DefAST - Class for variable & function definitions
class DefAST : public StmtAST {
	friend class Package;
	friend class Generator;
	friend class VarExprAST;
	friend class BlockAST;

	protected:
	std::string Name;

	public:
	DefAST(std::string name, const FTag &tag) : StmtAST(tag), Name(name) {}
	virtual ~DefAST() {}

	virtual TypeAST* typeAtDef() = 0;
};

class VarDefAST  : public DefAST {
	friend class Package;
	friend class Generator;
	friend class VarExprAST;
	friend class BlockAST;

	private:
	TypeAST *VType;
	ExprAST *Val;
	bool Var;
	public:
	VarDefAST(const FTag &tag, TypeAST *type, std::string name, ExprAST *value, bool var) :
		DefAST(name, tag),
		VType(type), Val(value), Var(var) {}

	virtual void typeCheck(Context *ctx);
	virtual TypeAST* typeAtDef();
};

// FuncDefAST - Class for function definitions
class FuncDefAST : public DefAST {
	friend class Package;
	friend class Generator;
	friend class VarExprAST;

	private:
	FuncExprAST *Val;
	
	public:
	virtual ~FuncDefAST() {}
	FuncDefAST(const FTag &tag, std::string name, FuncExprAST *val) :
		DefAST(name, tag), 
		Val(val) {}

	virtual void typeCheck(Context *ctx);
	virtual TypeAST* typeAtDef();
};

// FuncDefAST - Class for function definitions
class ExternFuncDefAST : public DefAST {
	friend class Package;
	friend class Generator;
	friend class VarExprAST;

	private:
	ExternAST *Val;
	TypeAST *EType;
	
	public:
	virtual ~ExternFuncDefAST() {}
	ExternFuncDefAST(const FTag &tag, std::string name, ExternAST *val, TypeAST *type) :
		DefAST(name, tag), 
		Val(val), EType(type) {}

	virtual void typeCheck(Context *ctx);
	virtual TypeAST* typeAtDef();
};


#endif
