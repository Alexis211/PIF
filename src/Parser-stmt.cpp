#include "Parser.h"


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
		return (ImportAST*) error("Expected package identifier after 'import'.");
	std::string as = path[path.size() - 1];
	if (Lex.tok == tok_as) {
		Lex.gettok(); // eat 'as'
		if (Lex.tok != tok_identifier)
			return (ImportAST*) error("Expected identifier after 'as'.");
		as = Lex.tokStr;
		Lex.gettok(); // eat identifier
	}
	return new ImportAST(tag, path, as);
}

// definition			(definition of a variable)
//		::= let id ':' type '=' expression 
//		::= let id '=' fctproto fctblock
//		::= let id '=' expression
DefAST *Parser::ParseVarDefinition() {
	FTag tag = Lex.tag();

	bool var = (Lex.tokStr == "var");

	Lex.gettok();		// eat 'let' or 'var'

	if (Lex.tok != tok_identifier) return (VarDefAST*) error("Expected identifier after 'let'/'var'.");
	std::string name = Lex.tokStr;
	Lex.gettok();		// eat identifier
	if (Lex.tokStr == ":") {
		Lex.gettok();	// eat ':'
		TypeAST *type = ParseType();
		if (type == 0) return 0;
		if (Lex.tokStr != "=") return (VarDefAST*) error("Expected '= expression' in 'let'/'var'.");
		Lex.gettok();	// eat '='
		ExprAST *val = ParseExpression();
		if (val == 0) return 0;
		return new VarDefAST(tag, type, name, val, var); 
	} else if (Lex.tokStr == "=") {
		Lex.gettok();		// eat '='
		ExprAST *val = ParseExpression();
		if (val == 0) return 0;
		return new VarDefAST(tag, 0, name, val, var);
	} else {
		return (VarDefAST*) error("Expected either ': type' or '= expression' after 'let/var " + name + "'.");
	}
};

DefAST *Parser::ParseFuncDefinition() {
	FTag tag = Lex.tag();	// definitions usually span several lines, so save tag for now.

	Lex.gettok();		// eat 'func'
	if (Lex.tok != tok_identifier) return (FuncDefAST*) error("Expected identifier after 'func'.");
	std::string name = Lex.tokStr;
	Lex.gettok();		// eat function name
	if (Lex.tokStr != ":") return (FuncDefAST*) error("Expected ':' in function definition.");
	Lex.gettok();

	FuncTypeAST *type = ParsePrototype();
	if (type == 0) return 0;
	if (Lex.tok == tok_extern) {
		ExternAST *v = dynamic_cast<ExternAST*>(ParseExtern());
		return new ExternFuncDefAST(tag, name, v, type);
	} else {
		BlockAST *b = dynamic_cast<BlockAST*>(ParseBlock());
		if (b == 0) return 0;
		return new FuncDefAST(tag, name, new FuncExprAST(tag, type, b));
	}
}
