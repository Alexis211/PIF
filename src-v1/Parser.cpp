#include "Parser.h"


// ERRORS

ImportAST *Parser::errorI(const std::string &msg) {
	std::cerr << Lex.tag().str() << " Import Error: " << msg << std::endl;
	return 0;
}

ExprAST *Parser::errorE(const std::string &msg) {
	std::cerr << Lex.tag().str() << " Expression Error: " << msg << std::endl;
	return 0;
}

DefAST *Parser::errorD(const std::string &msg) {
	std::cerr << Lex.tag().str() << " Definition Error: " << msg << std::endl;
	return 0;
}

TypeAST *Parser::errorT(const std::string &msg) {
	std::cerr << Lex.tag().str() << " Type Error: " << msg << std::endl;
	return 0;
}

FuncTypeAST *Parser::errorFT(const std::string &msg) {
	std::cerr << Lex.tag().str() << " Function Type Error: " << msg << std::endl;
	return 0;
}

FuncArgAST *Parser::errorFA(const std::string &msg) {
	std::cerr << Lex.tag().str() << " Function Argument Error: " << msg << std::endl;
	return 0;
}

// Expression parsing

// import
//		::= 'import' path* package
//		::= 'import' path* package 'as' name
ImportAST *Parser::ParseImport() {
	FTag tag = Lex.tag();

	Lex.gettok();	// eat 'import'
	std::vector<std::string> path;
	while (Lex.tok == tok_identifier) {
		path.push_back(Lex.tokStr);
		Lex.gettok();
		if (Lex.tokStr == ".") Lex.gettok();
		else break;
	}
	if (path.size() == 0)
		return errorI("Expected package identifier after 'import'.");
	std::string as = path[path.size() - 1];
	if (Lex.tok == tok_as) {
		Lex.gettok(); // eat 'as'
		if (Lex.tok != tok_identifier)
			return errorI("Expected identifier after 'as'.");
		as = Lex.tokStr;
		Lex.gettok(); // eat identifier
	}
	return new ImportAST(tag, path, as);
}

// boolexpr ::= 'true' | 'false'
ExprAST *Parser::ParseBoolExpr() {
	ExprAST *result = new BoolExprAST(Lex.tag(), (Lex.tok == tok_true ? true : false));
	Lex.gettok();
	return result;
}

// intexpr ::= int
ExprAST *Parser::ParseIntExpr() {
	ExprAST *result = new IntExprAST(Lex.tag(), Lex.tokInt);
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
	if (!v) return 0;
	if (Lex.tokStr != ")") return errorE("Expected ')'.");
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
	if (Lex.tokStr == "(") return ParseParenExpr();
	ExprAST *ret = errorE("Unknown token '" + Lex.tokStr + "' when expecting an expression.");
	Lex.gettok();
	return ret;
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
		ExprAST *v = ParseExpression();
		if (v == 0) return 0;
		return new ReturnAST(Lex.tag(), v);
	} else {
		ExprAST *LHS = ParsePrimary();
		if (!LHS) return 0;
		return ParseBinOpRHS(0, LHS);
	}
	return 0;
}

// binoprhs
//		::= ('+' primary)*
//		::= functioncall
ExprAST *Parser::ParseBinOpRHS(int ExprPrec, ExprAST *LHS) {
	while (1) {
		int tokPrec = getBinopPrec(Lex.tokStr);

		if (tokPrec < ExprPrec) return LHS;

		if (Lex.tokStr == "(") {		// this isn't really a binary expression, it's a function call
			LHS = ParseCall(LHS);
		} else {
			std::string binop = Lex.tokStr;
			Lex.gettok();

			ExprAST *RHS = ParsePrimary();
			if (!RHS) return 0;

			int nextPrec = getBinopPrec(Lex.tokStr);
			if (tokPrec < nextPrec) {
				RHS = ParseBinOpRHS(tokPrec + 1, RHS);
				if (!RHS) return 0;
			}

			LHS = new BinaryExprAST(Lex.tag(), binop, LHS, RHS);
		}
	}
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
			if (!arg) return 0;
			args.push_back(arg);

			if (Lex.tokStr == ")") break;
			if (Lex.tokStr != ",") return errorE("Expected ')' or ',' in parameter list.");
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
	if (Lex.tok != tok_identifier) return errorE("Expected identifier after 'extern', not '" + Lex.tokStr + "'.");
	std::string sym = Lex.tokStr;
	Lex.gettok();		// eat identifier
	TypeAST *type = 0;
	if (Lex.tokStr == ":") {
		Lex.gettok();	// eat ':'
		type = ParseType();
	}
	return new ExternAST(Lex.tag(), type, sym);
}

// block
//		::= (expression ';')*
ExprAST *Parser::ParseBlock() {
	std::vector<ExprAST *> expressions;
	Lex.gettok();	// eat '{'
	while (1) {
		if (Lex.tokStr == "}") break;
		ExprAST* e = ParseExpression();
		if (e == 0) return 0;
		expressions.push_back(e);
		if (Lex.tokStr == ";") Lex.gettok();	// eat ';'
	}
	Lex.gettok(); 		// eat '}'
	return new BlockAST(Lex.tag(), expressions);
}

// type
//		::= 'void'
//		::= 'int'
//		::= 'float'
//		::= prototype
TypeAST *Parser::ParseType() {
	if (Lex.tokStr == "void") {
		Lex.gettok();	// eat 'void'
		return &voidBaseType;
	}
	if (Lex.tokStr == "bool") {
		Lex.gettok();	// eat 'bool'
		return &boolBaseType;
	}
	if (Lex.tokStr == "int") {
		Lex.gettok();	// eat 'int'
		return &intBaseType;
	}
	if (Lex.tokStr == "float") {
		Lex.gettok();	// eat 'float'
		return &floatBaseType;
	}
	if (Lex.tokStr == "(") {
		return ParsePrototype();
	}
	return errorT("Invalid type definiton, starting by '" + Lex.tokStr + "'.");
}

// funcarg
//		::= id ':' type
//		::= id ':' type '=' default 	TODO!!
FuncArgAST *Parser::ParseFuncArg() {
	if (Lex.tok != tok_identifier) return errorFA("Expected identifier for argument name.");
	std::string name = Lex.tokStr;
	Lex.gettok();		// eat identifier
	if (Lex.tokStr != ":") return errorFA("Expected argument type.");
	Lex.gettok();		// eat ':'
	TypeAST *type = ParseType();
	if (type == 0) return 0;
	return new FuncArgAST(name, type);
}

// prototype
//		::= '(' funcarg* ')' '->' type*
//		::= '(' funcarg* ')' '->'		 ONLY IF FOLLOWED BY '{', ELSE INVALID
FuncTypeAST *Parser::ParsePrototype() {
	Lex.gettok(); 	// eat '('
	std::vector<FuncArgAST *> args;
	if (Lex.tokStr != ")") {
		while (1) {
			FuncArgAST *arg = ParseFuncArg();
			if (!arg) return 0;
			args.push_back(arg);

			if (Lex.tokStr == ")") break;
			if (Lex.tokStr != ",") return errorFT("Expected ')' or ',' in argument list.");
			Lex.gettok();
		}
	}

	Lex.gettok();	// eat ')'
	if (Lex.tokStr != "->") return errorFT("Invalid prototype : was expecting '->', not '" + Lex.tokStr + "'.");
	Lex.gettok();	// eat '->'
	if (Lex.tokStr == "{") {
		return new FuncTypeAST(args, &voidBaseType);
	}
	TypeAST *retType = ParseType();
	if (retType == 0) return 0;
	return new FuncTypeAST(args, retType);
}

// definition			(definition of a variable)
//		::= let id ':' type '=' expression 
//		::= let id '=' fctproto fctblock
//		::= let id '=' expression
DefAST *Parser::ParseDefinition() {
	FTag tag = Lex.tag();	// definitions usually span several lines, so save tag for now.

	Lex.gettok();		// eat 'let'
	if (Lex.tok != tok_identifier) return errorD("Expected identifier after 'let'.");
	std::string name = Lex.tokStr;
	Lex.gettok();		// eat identifier
	if (Lex.tokStr == ":") {
		Lex.gettok();	// eat ':'
		TypeAST *type = ParseType();
		if (type == 0) return 0;
		if (Lex.tokStr != "=") return errorD("Expected '= expression' in 'let'.");
		Lex.gettok();	// eat '='
		ExprAST *val = ParseExpression();
		if (ExternAST *extrn = dynamic_cast<ExternAST*>(val)) {
			if (extrn->Type != 0) return errorD("Redundant definition of extern type");
			extrn->Type = type;
		}
		if (val == 0) return 0;
		return new DefAST(tag, type, name, val); 
	} else if (Lex.tokStr == "=") {
		Lex.gettok();		// eat '='
		if (Lex.tokStr == "(") {
			FuncTypeAST *type = ParsePrototype();
			if (type == 0) return 0;
			ExprAST *val = ParseBlock();
			if (val == 0) return 0;
			if (BlockAST *code = dynamic_cast<BlockAST*>(val)) {
				return new DefAST(tag, type, name, new FuncExprAST(tag, type, code));
			} else {
				std::vector<ExprAST*> instr;
				instr.push_back(val);
				BlockAST *block = new BlockAST(tag, instr);
				return new DefAST(tag, type, name, new FuncExprAST(tag, type, block));
			}
		} else {
			ExprAST *val = ParseExpression();
			if (val == 0) return 0;
			return new DefAST(tag, 0, name, val);
		}
	} else {
		return errorD("Expected either ': type' or '= expression' after 'let " + name + "'.");
	}
};
