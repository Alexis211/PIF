#include "Parser.h"
#include "../error.h"

// Expression parsing

// boolexpr ::= 'true' | 'false'
ExprAST *Parser::ParseBoolExpr() {
	ExprAST *result = new BoolExprAST(Lex.tag(), (Lex.tok == tok_true ? true : false));
	Lex.gettok();
	return result;
}

// intexpr ::= int
ExprAST *Parser::ParseIntExpr() {
	ExprAST *result = new IntExprAST(Lex.tag(), Lex.tokInt, INTTYPE);
	Lex.gettok();
	return result;
}

// floatexpr ::= float
ExprAST *Parser::ParseFloatExpr() {
	ExprAST *result = new FloatExprAST(Lex.tag(), Lex.tokFloat);
	Lex.gettok();
	return result;
}

// parenexpr ::= '(' expression ')'
ExprAST *Parser::ParseParenExpr() {
	Lex.gettok();	// eat '('
	ExprAST *v = ParseExpression();
	if (Lex.tokStr != ")") Lex.tag().Throw("Expected ')'.");
	Lex.gettok();	// eat ')'
	return v;
}


// identifierexpr
//		::= identifier
//		::= identifier '(' expression* ')'
ExprAST *Parser::ParseIdentifierExpr() {
	std::string idname = Lex.tokStr;
	Lex.gettok();		// eat identifier

	return new VarExprAST(Lex.tag(), idname);
}

// primary
//		::= identifierexpr
//		::= floatexpr
//		::= intexpr
//		::= boolexpr
//		::= parenexpr
//		::= externexpr
ExprAST *Parser::ParsePrimary() {
	if (Lex.tok == tok_extern) return ParseExtern();
	if (Lex.tok == tok_identifier) return ParseIdentifierExpr();
	if (Lex.tok == tok_true || Lex.tok == tok_false) return ParseBoolExpr();
	if (Lex.tok == tok_int) return ParseIntExpr();
	if (Lex.tok == tok_float) return ParseFloatExpr();
	if (Lex.tok == tok_if) return ParseIfThenElse();
	if (Lex.tok == tok_while || Lex.tok == tok_until) return ParseWhile();
	if (Lex.tok == tok_break || Lex.tok == tok_continue) {
		BreakContE t = (Lex.tok == tok_break ? bc_break : bc_continue);
		Lex.gettok();
		return new BreakContAST(Lex.tag(), t);
	}
	if (Lex.tokStr == "(") return ParseParenExpr();
	Lex.tag().Throw("Unknown token '" + Lex.tokStr + "' when expecting an expression.");
	return 0;
}


// expression
//		::= primary binoprhs
//		::= primary binoprhs '(' arguments ')'
//		::= block
ExprAST *Parser::ParseExpression() {
	if (Lex.tokStr == "{") {
		return ParseBlock();
	} else if (Lex.tok == tok_return) {
		Lex.gettok();
		if (Lex.tokStr == "}" || Lex.tokStr == ";") {
			return new ReturnAST(Lex.tag(), 0);
		} else {
			ExprAST *v = ParseExpression();
			return new ReturnAST(Lex.tag(), v);
		}
	} else {
		ExprAST *LHS = ParseBinOpLHS();
		return ParseBinOpRHS(0, LHS);
	}
}

// binoplhs
//		::= 
//		::= primary
ExprAST *Parser::ParseBinOpLHS() {
	if (Lex.tokStr == "-" || Lex.tokStr == "@" || Lex.tokStr == "!") {
		return 0;
	} else {
		return ParsePrimary();
	}
}

// binoprhs
//		::= ('+' primary)*
//		::= functioncall
ExprAST *Parser::ParseBinOpRHS(int ExprPrec, ExprAST *LHS) {
	while (1) {
		int tokPrec = getBinopPrec(Lex.tokStr);

		if (tokPrec < ExprPrec) {
			if (LHS == 0) throw new PIFError("Bad LHS.");
			return LHS;
		}

		if (Lex.tokStr == "(") {		// this isn't really a binary expression, it's a function call
			LHS = ParseCall(LHS);
			if (LHS == 0) return 0;
		} else if (Lex.tokStr == ":") {	// this isn't really a binary expression, it's a cast operation
			Lex.gettok();
			TypeAST *Type = ParseType();
			LHS = new CastExprAST(Lex.tag(), LHS, Type);
		} else if (Lex.tokStr == ".") {
			LHS = ParseDotMember(LHS);
			if (LHS == 0) return 0;
		} else {
			std::string binop = Lex.tokStr;
			Lex.gettok();

			ExprAST *RHS = ParseBinOpLHS();

			int nextPrec = getBinopPrec(Lex.tokStr);
			if (tokPrec < nextPrec) {
				RHS = ParseBinOpRHS(tokPrec + 1, RHS);
				if (!RHS) return 0;
			}
			
			if (LHS == 0) {
				if (binop == "@") {
					LHS = new DerefExprAST(Lex.tag(), RHS);
				} else {
					LHS = new UnaryExprAST(Lex.tag(), binop, RHS);
				}
			} else {
				LHS = new BinaryExprAST(Lex.tag(), binop, LHS, RHS);
			}
		}
	}
}

// dotmember
//		::= '.' identifier
ExprAST *Parser::ParseDotMember(ExprAST *obj) {
	Lex.gettok();	// eat '.'
	if (Lex.tok != tok_identifier) Lex.tag().Throw("Expected identifier after '.'.");
	std::string id = Lex.tokStr;
	Lex.gettok();

	return new DotMemberExprAST(Lex.tag(), obj, id);
}

// call
//		::= '(' args* ')'
ExprAST *Parser::ParseCall(ExprAST *callee) {
	// Function call
	Lex.gettok(); //eat '('
	std::vector<ExprAST *> args;
	if (Lex.tokStr != ")") {
		while (1) {
			ExprAST *arg = ParseExpression();
			args.push_back(arg);

			if (Lex.tokStr == ")") break;
			if (Lex.tokStr != ",") Lex.tag().Throw("Expected ')' or ',' in parameter list.");
			Lex.gettok();
		}
	}
	Lex.gettok();	// eat ')'
	return new CallExprAST(Lex.tag(), callee, args);
}

// externexpr
//		::= extern symbol_name
//		::= extern symbol_name ':' type
ExprAST *Parser::ParseExtern() {
	Lex.gettok();		// eat 'extern'
	if (Lex.tok != tok_identifier) Lex.tag().Throw("Expected identifier after 'extern', not '" + Lex.tokStr + "'.");
	std::string sym = Lex.tokStr;
	Lex.gettok();		// eat identifier
	TypeAST *type = 0;
	/*if (Lex.tokStr == ":") {
		Lex.gettok();	// eat ':'
		type = ParseType();
	}*/
	return new ExternAST(Lex.tag(), type, sym);
}

// ifthenelse
//		::= 'if' expression 'then' expression 'else' expression
//		::= 'if' expression block 'else' expression
ExprAST *Parser::ParseIfThenElse() {
	FTag tag = Lex.tag();

	Lex.gettok();	// eat if

	ExprAST *cond = ParseExpression();

	if (Lex.tok == tok_then) Lex.gettok();
	ExprAST *trueE = ParseExpression();

	ExprAST *falseE = 0;
	if (Lex.tok == tok_else) {
		Lex.gettok();
		falseE = ParseExpression();
	}

	return new IfThenElseAST(tag, cond, trueE, falseE);
}

// while
//		::= 'while/until' expr 'do' expr
//		::= 'while/until' expr block
ExprAST *Parser::ParseWhile() {
	FTag tag = Lex.tag();

	bool isUntil = (Lex.tok == tok_until);
	Lex.gettok();		// eat while or until

	ExprAST *cond = ParseExpression();
	
	if (Lex.tok == tok_do) Lex.gettok();
	ExprAST *cont = ParseExpression();

	return new WhileAST(tag, cond, cont, isUntil);
}

// block
//		::= (expression ';')*
ExprAST *Parser::ParseBlock() {
	std::vector<StmtAST *> instructions;
	Lex.gettok();	// eat '{'
	while (1) {
		FTag tag = Lex.tag();
		if (Lex.tokStr == "}") break;
		StmtAST *s = 0;
		if (Lex.tok == tok_var || Lex.tok == tok_let) {
			s = ParseVarDefinition();
		} else {
			ExprAST* e = ParseExpression();
			s = new ExprStmtAST(tag, e);
		}
		instructions.push_back(s);
		if (Lex.tokStr == ";") Lex.gettok();	// eat ';'
	}
	Lex.gettok(); 		// eat '}'
	return new BlockAST(Lex.tag(), instructions);
}


