#ifndef DEF_PARSER_H
#define DEF_PARSER_H

#include "AST-expr.h"
#include "AST-stmt.h"
#include "Lexer.h"

class Parser {
	private:
	Lexer &Lex;
	std::map<std::string, int> BinopPrecedence;

	// Errors
	void* error(const std::string &msg);

	ExprAST *ParseBoolExpr();
	ExprAST *ParseIntExpr();
	ExprAST *ParseFloatExpr();
	ExprAST *ParseParenExpr();
	ExprAST *ParseIdentifierExpr();
	ExprAST *ParsePrimary();
	ExprAST *ParseBinOpLHS();
	ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS);
	ExprAST *ParseDotMember(ExprAST *obj);
	ExprAST *ParseCall(ExprAST *callee);
	ExprAST *ParseExtern();
	ExprAST *ParseIfThenElse();
	ExprAST *ParseWhile();

	ExprAST *ParseBlock();

	ExprAST *ParseExpression();

	TypeAST *ParseType();
	FuncArgAST *ParseFuncArg();
	FuncTypeAST *ParsePrototype();

	public:
	Parser(Lexer &l) : Lex(l) {
		setBinopPrec("=", 2);
		setBinopPrec("||", 10);
		setBinopPrec("&&", 10);
		setBinopPrec("!", 20);
		setBinopPrec("<", 30);
		setBinopPrec(">", 30);
		setBinopPrec("<=", 30);
		setBinopPrec(">=", 30);
		setBinopPrec("==", 30);
		setBinopPrec("!=", 30);
		setBinopPrec("+", 50);
		setBinopPrec("-", 50);
		setBinopPrec("*", 60);
		setBinopPrec("/", 60);
		setBinopPrec("%", 60);
		setBinopPrec(":", 80);
		setBinopPrec("(", 100); // Everything above 100 has priority over a function-call expression
		setBinopPrec(".", 120);
		setBinopPrec("@", 130);
	}

	DefAST *ParseVarDefinition();
	DefAST *ParseFuncDefinition();
	ImportAST *ParseImport();

	void setBinopPrec(std::string op, int prec) {
		BinopPrecedence[op] = prec;
	}
	int getBinopPrec(std::string op) {
		if (!isascii(op[0])) return -1;
		int ret = BinopPrecedence[op];
		if (ret <= 0) ret = -1;
		return ret;
	}
};


#endif

