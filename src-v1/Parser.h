#ifndef DEF_PARSER_H
#define DEF_PARSER_H

#include "AST.h"
#include "Lexer.h"

class Parser {
	private:
	Lexer &Lex;
	std::map<std::string, int> BinopPrecedence;

	// Errors
	ImportAST *errorI(const std::string &msg);
	ExprAST *errorE(const std::string &msg);
	DefAST *errorD(const std::string &msg);
	TypeAST *errorT(const std::string &msg);
	FuncTypeAST *errorFT(const std::string &msg);
	FuncArgAST *errorFA(const std::string &msg);

	ExprAST *ParseBoolExpr();
	ExprAST *ParseIntExpr();
	ExprAST *ParseFloatExpr();
	ExprAST *ParseParenExpr();
	ExprAST *ParseIdentifierExpr();
	ExprAST *ParsePrimary();
	ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS);
	ExprAST *ParseCall(ExprAST *callee);
	ExprAST *ParseExtern();

	ExprAST *ParseBlock();

	TypeAST *ParseType();
	FuncArgAST *ParseFuncArg();
	FuncTypeAST *ParsePrototype();

	public:
	Parser(Lexer &l) : Lex(l) {
		setBiopPrec("||", 10);
		setBiopPrec("&&", 10);
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
		setBinopPrec("(", 100); // Everything above 100 has priority over a function-call expression
		setBinopPrec(".", 120);
	}

	DefAST *ParseDefinition();
	ExprAST *ParseExpression();
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

