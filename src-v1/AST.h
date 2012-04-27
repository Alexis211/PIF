#ifndef DEF_AST_H
#define DEF_AST_H

#include "config.h"

#include "Lexer.h"

#include <string>
#include <map>
#include <vector>
#include <iostream>

#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>

class Context;

// ImportAST - Class for import statements
class ImportAST {
	friend class Package;
	private:
	std::vector<std::string> Path;
	std::string As;

	public:
	virtual ~ImportAST() {}
	ImportAST(const FTag &tag, std::vector<std::string> path, std::string as) : Path(path), As(as), Tag(tag) {}

	const FTag Tag;
};

// TypeAST - Base class for all type nodes
class TypeAST {
	public:
	virtual ~TypeAST() {}
	virtual llvm::Type *getTy() = 0;

	virtual void prettyprint(std::ostream &out) = 0;
};

// BaseTypeAST - Class for types like int or float
enum BaseTypeE {
	bt_void,
	bt_bool,
	bt_int,
	bt_float,
};
class BaseTypeAST : public TypeAST {
	BaseTypeE BaseType;
	public:
	BaseTypeAST(BaseTypeE basetype) : BaseType(basetype) {}
	virtual llvm::Type *getTy();

	virtual void prettyprint(std::ostream &out);
};

extern BaseTypeAST voidBaseType, boolBaseType, intBaseType, floatBaseType;

// FuncArgAST - Class for function arguments in definitions
class FuncArgAST {
	friend class FuncTypeAST;
	friend class Generator;
	friend class CallExprAST;

	std::string Name;
	TypeAST* ArgType;
	public:
	FuncArgAST(std::string name, TypeAST *type) : Name(name), ArgType(type) {}

	virtual void prettyprint(std::ostream &out);
};


// FuncTypeAST - Class for function types
class FuncTypeAST : public TypeAST {
	friend class Generator;
	friend class ExternAST;
	friend class CallExprAST;

	std::vector<FuncArgAST*> Args;
	TypeAST* ReturnType;
	public:
	FuncTypeAST(std::vector<FuncArgAST*> &args, TypeAST* retType) : Args(args), ReturnType(retType) {};
	virtual llvm::Type *getTy();

	virtual void prettyprint(std::ostream &out);
};


// ExprAST - Base class for all expression nodes
class ExprAST {
	public:
	ExprAST(const FTag &tag) : Tag(tag) {}

	virtual ~ExprAST() {}
	virtual TypeAST* type(Context *ctx) = 0;
	virtual llvm::Value *Codegen(Context *ctx) = 0;

	virtual void prettyprint(std::ostream &out) = 0;

	const FTag Tag;
};

// BoolExprAST - Expression class for booleans true and false
class BoolExprAST : public ExprAST {
	bool Val;
	public:
	BoolExprAST(const FTag &tag, bool val) : ExprAST(tag), Val(val) {}
	TypeAST* type(Context *ctx);
	virtual llvm::Value *Codegen(Context *ctx);

	virtual void prettyprint(std::ostream &out);
};

// IntExprAST - Expression class for integer like "42" or "122222452"
class IntExprAST : public ExprAST {
	INT Val;
	public:
	IntExprAST(const FTag &tag, INT val): ExprAST(tag), Val(val) {}
	TypeAST* type(Context *ctx);
	virtual llvm::Value *Codegen(Context *ctx);

	virtual void prettyprint(std::ostream &out);
};

// FloatExprAST - Expression class for floats like "2.0" or "3.14"
class FloatExprAST : public ExprAST {
	FLOAT Val;
	public:
	FloatExprAST(const FTag &tag, FLOAT val): ExprAST(tag), Val(val) {}
	TypeAST* type(Context *ctx);
	virtual llvm::Value *Codegen(Context *ctx);

	virtual void prettyprint(std::ostream &out);
};

// VarExprAST - Expression class for referencing a variable, like "a"
class VarExprAST : public ExprAST {
	std::string Name;
	public:
	VarExprAST(const FTag &tag, const std::string &name) : ExprAST(tag), Name(name) {}
	TypeAST* type(Context *ctx);
	virtual llvm::Value *Codegen(Context *ctx);

	virtual void prettyprint(std::ostream &out);
};

// BinaryExprAST - Expression class for binary operators (ex: "a + b")
class BinaryExprAST : public ExprAST {
	std::string Op;
	ExprAST *LHS, *RHS;
	public:
	BinaryExprAST(const FTag &tag, std::string op, ExprAST *lhs, ExprAST *rhs) :
		ExprAST(tag), Op(op), LHS(lhs), RHS(rhs) {}
	TypeAST *type(Context *ctx);
	virtual llvm::Value *Codegen(Context *ctx);

	virtual void prettyprint(std::ostream &out);
};

// CallExprAST - Expression class for a function call
class CallExprAST : public ExprAST {
	ExprAST *Callee;
	std::vector<ExprAST*> Args;
	public:
	CallExprAST(const FTag &tag, ExprAST* callee, std::vector<ExprAST*> &args) :
		ExprAST(tag), Callee(callee), Args(args) {}
	TypeAST *type(Context *ctx);
	virtual llvm::Value *Codegen(Context *ctx);

	virtual void prettyprint(std::ostream &out);
};

// BlockAST - Class for instruction blocks
class BlockAST : public ExprAST{
	std::vector<ExprAST*> Instructions;
	public:
	BlockAST(const FTag &tag, std::vector<ExprAST*> instr) : ExprAST(tag), Instructions(instr) {}
	TypeAST* type(Context *ctx);
	virtual llvm::Value *Codegen(Context *ctx);
	void FctEntryCodegen(Context *ctx);

	virtual void prettyprint(std::ostream &out);
};

// ReturnAST - Class for return statements
class ReturnAST : public ExprAST {
	ExprAST* Val;
	public:
	ReturnAST(const FTag &tag, ExprAST* val) : ExprAST(tag), Val(val) {}
	TypeAST* type(Context *ctx);
	virtual llvm::Value *Codegen(Context *ctx);

	virtual void prettyprint(std::ostream &out);
};

// FuncExprAST - Class for function expressions - right, functions are expressions like any other expression
class FuncExprAST : public ExprAST {
	friend class Generator;

	FuncTypeAST *Type;
	BlockAST *Code;
	public:
	FuncExprAST(const FTag &tag, FuncTypeAST *type, BlockAST *code) : ExprAST(tag), Type(type), Code(code) {}
	TypeAST* type(Context *ctx);
	virtual llvm::Value *Codegen(Context *ctx);

	virtual void prettyprint(std::ostream &out);
};

// ExternAST - Class for extern expressions
class ExternAST : public ExprAST {
	friend class Parser;

	TypeAST *Type;
	std::string Symbol;
	public:
	ExternAST(const FTag &tag, TypeAST *type, std::string sym) : 
		ExprAST(tag), Type(type), Symbol(sym) {}
	TypeAST* type(Context *ctx);
	virtual llvm::Value *Codegen(Context *ctx);

	virtual void prettyprint(std::ostream &out);
};

// DefAST - Class for variable and function definitions
class DefAST {
	friend class Package;
	friend class Generator;
	friend class VarExprAST;
	private:
	TypeAST *Type;
	std::string Name;
	ExprAST *Val;
	public:
	virtual ~DefAST() {}
	DefAST(const FTag &tag, TypeAST *type, std::string name, ExprAST *value) :
		Type(type), Name(name), Val(value), Tag(tag) {}

	virtual void prettyprint(std::ostream &out);

	const FTag Tag; 
};

#endif

