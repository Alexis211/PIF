#ifndef DEF_AST_H
#define DEF_AST_H

#include "config.h"

#include "Lexer.h"

#include <string>
#include <map>
#include <vector>
#include <iostream>


#include "AST-type.h"

class Context;

// ExprAST - Base class for all expression nodes
class ExprAST {
	private:
	bool dep_loop;
	protected:
	Context *Ctx;
	TypeAST *EType;
	virtual TypeAST* getType() = 0;
	public:
	ExprAST(const FTag &tag) : dep_loop(false), Ctx(0), EType(NULL), Tag(tag) {}

	virtual ~ExprAST() {}
	TypeAST *type(Context *ctx);
	virtual ExprAST *asType(TypeAST *ty);
	ExprAST *asTypeOrError(TypeAST *ty);

	virtual llvm::Value *Codegen() = 0;

	virtual void prettyprint(std::ostream &out) = 0;

	const FTag Tag;

	void* error(const std::string &msg);

	ExprAST* cannotConvert(TypeAST *t);
};

// BoolExprAST - Expression class for booleans true and false
class BoolExprAST : public ExprAST {
	bool Val;
	TypeAST* getType();
	public:
	BoolExprAST(const FTag &tag, bool val) : ExprAST(tag), Val(val) {}
	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);
};

// IntExprAST - Expression class for integer like "42" or "122222452"
class IntExprAST : public ExprAST {
	INT Val;
	IntTypeAST *IType;
	TypeAST* getType();
	public:
	IntExprAST(const FTag &tag, INT val, IntTypeAST *ty): ExprAST(tag), Val(val), IType(ty) {}
	virtual llvm::Value *Codegen();

	virtual ExprAST *asType(TypeAST *ty);

	virtual void prettyprint(std::ostream &out);
};

// FloatExprAST - Expression class for floats like "2.0" or "3.14"
class FloatExprAST : public ExprAST {
	FLOAT Val;
	TypeAST* getType();
	public:
	FloatExprAST(const FTag &tag, FLOAT val): ExprAST(tag), Val(val) {}
	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);
};

// VarExprAST - Expression class for referencing a variable, like "a"
class Symbol;
class VarExprAST : public ExprAST {
	std::string Name;
	Symbol *Sym;
	bool IsGlobalConst;

	TypeAST* getType();
	public:
	VarExprAST(const FTag &tag, const std::string &name) : ExprAST(tag), Name(name), Sym(0), IsGlobalConst(false) {}
	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);
};

// DerefExprAST -  Expression class for variable dereferencments
class DerefExprAST : public ExprAST {
	ExprAST *Val;
	TypeAST* getType();
	public:
	DerefExprAST(const FTag &tag, ExprAST *val) : ExprAST(tag), Val(val) {}
	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);
};

// UnaryExprAST - Expression class for unary operators (ex: "-a" or "!a")
class UnaryExprAST : public ExprAST {
	std::string Op;
	ExprAST *Expr;
	TypeAST *getType();
	public:
	UnaryExprAST(const FTag &tag, std::string op, ExprAST *expr) :
		ExprAST(tag), Op(op), Expr(expr) {}
	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);
};

// BinaryExprAST - Expression class for binary operators (ex: "a + b")
class BinaryExprAST : public ExprAST {
	std::string Op;
	ExprAST *LHS, *RHS;
	TypeAST *getType();
	public:
	BinaryExprAST(const FTag &tag, std::string op, ExprAST *lhs, ExprAST *rhs) :
		ExprAST(tag), Op(op), LHS(lhs), RHS(rhs) {}
	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);
};

// DotMemberExprAST - Expression class for referencng a member with 'object.member' syntax
class DotMemberExprAST : public ExprAST {
	ExprAST *Obj;
	std::string Member;
	TypeAST *getType();
	public:
	DotMemberExprAST(const FTag &tag, ExprAST *obj, std::string member) :
		ExprAST(tag), Obj(obj), Member(member) {}
	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);

};

// CastExprAST - Expression class for type casts with operator ':'
class CastExprAST : public ExprAST {
	ExprAST *Expr;
	TypeAST *FType;
	bool NeedCast;
	TypeAST *getType();
	public:
	CastExprAST(const FTag &tag, ExprAST *expr, TypeAST *type) :
		ExprAST(tag), Expr(expr), FType(type), NeedCast(true) {};
	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);
};

// CallExprAST - Expression class for a function call
class CallExprAST : public ExprAST {
	ExprAST *Callee;
	std::vector<ExprAST*> Args;
	TypeAST *getType();
	public:
	CallExprAST(const FTag &tag, ExprAST* callee, const std::vector<ExprAST*> &args) :
		ExprAST(tag), Callee(callee), Args(args) {}
	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);
};

// IfThenElseAST - Expression class for an if-then-else construct
class IfThenElseAST : public ExprAST {
	ExprAST *Cond, *TrueBr, *FalseBr;

	TypeAST *getType();
	public:
	IfThenElseAST(const FTag &tag, ExprAST *cond, ExprAST *truebr, ExprAST *falsebr) :
		ExprAST(tag), Cond(cond), TrueBr(truebr), FalseBr(falsebr) {}
	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);
};

// WhileAST - Expression class for while/until constructs
class WhileAST : public ExprAST {
	ExprAST *Cond, *Inside;
	bool IsUntil;

	TypeAST *getType();
	public:
	WhileAST(const FTag &tag, ExprAST *cond, ExprAST *inside, bool isuntil) :
		ExprAST(tag), Cond(cond), Inside(inside), IsUntil(isuntil) {}
	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);
};

// BreakContAST - Expression class for break & continue statments
enum BreakContE {
	bc_break,
	bc_continue,
};
class BreakContAST : public ExprAST {
	BreakContE SType;

	TypeAST* getType();
	public:
	BreakContAST(const FTag &tag, BreakContE st) : ExprAST(tag), SType(st) {}
	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);
};

// BlockAST - Class for instruction blocks
class StmtAST;
class BlockAST : public ExprAST{
	std::vector<StmtAST*> Instructions;

	bool OwnContext;

	TypeAST* getType();
	public:
	BlockAST(const FTag &tag, const std::vector<StmtAST*> &instr) : 
		ExprAST(tag), Instructions(instr), OwnContext(false) {}
	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);
};

// ReturnAST - Class for return statements
class ReturnAST : public ExprAST {
	ExprAST* Val;
	TypeAST* getType();
	public:
	ReturnAST(const FTag &tag, ExprAST* val) : ExprAST(tag), Val(val) {}
	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);
};

// FuncExprAST - Class for function expressions - right, functions are expressions like any other expression
class FuncExprAST : public ExprAST {
	friend class Generator;
	friend class VarExprAST;
	friend class FuncDefAST;

	FuncTypeAST *FType;
	BlockAST *Code;
	bool OwnContext;

	TypeAST* getType();

	public:
	FuncExprAST(const FTag &tag, FuncTypeAST *type, BlockAST *code) :
		ExprAST(tag), FType(type), Code(code), OwnContext(false) {}


	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);
};

// ExternAST - Class for extern expressions
class ExternAST : public ExprAST {
	friend class Parser;
	friend class ExternFuncDefAST;
	friend class CastExprAST;

	TypeAST *SType;
	std::string Symbol;
	TypeAST* getType();
	public:
	ExternAST(const FTag &tag, TypeAST *type, std::string sym) : 
		ExprAST(tag), SType(type), Symbol(sym) {}


	virtual ExprAST *asType(TypeAST *ty);

	virtual llvm::Value *Codegen();

	virtual void prettyprint(std::ostream &out);
};

#endif

