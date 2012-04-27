#include "Parser.h"


// type
//		::= 'void'
//		::= 'int'
//		::= 'float'
//		::= prototype
TypeAST *Parser::ParseType() {
	if (Lex.tokStr == "(") {
		return ParsePrototype();
	}
	if (Lex.tokStr == "&") {
		Lex.gettok();
		TypeAST *t = ParseType();
		if (t == 0) return 0;
		return RefTypeAST::Get(t);
	}
	TypeAST *ret = 0;
	if (Lex.tokStr == "void") ret = &voidBaseType;
	if (Lex.tokStr == "bool") ret = &boolBaseType;
	if (Lex.tokStr == "float") ret = &floatBaseType;
	if (Lex.tokStr == "s64i" || Lex.tokStr == "int") ret = &s64iBaseType;
	if (Lex.tokStr == "u8i" || Lex.tokStr == "byte") ret = &u8iBaseType;
	if (Lex.tokStr == "u16i") ret = new IntTypeAST(16, false);
	if (Lex.tokStr == "u32i") ret = new IntTypeAST(32, false);
	if (Lex.tokStr == "u64i") ret = new IntTypeAST(64, false);
	if (Lex.tokStr == "s8i") ret = new IntTypeAST(8, true);
	if (Lex.tokStr == "s16i") ret = new IntTypeAST(16, true);
	if (Lex.tokStr == "s32i") ret = new IntTypeAST(32, true);
	if (ret != 0) {
		Lex.gettok();
		return ret;
	}
	return (TypeAST*) error("Invalid type definiton, starting by '" + Lex.tokStr + "'.");
}



// funcarg
//		::= id ':' type
//		::= id ':' type '=' default 	TODO!!
FuncArgAST *Parser::ParseFuncArg() {
	if (Lex.tok != tok_identifier) return (FuncArgAST*) error("Expected identifier for argument name.");
	std::string name = Lex.tokStr;
	Lex.gettok();		// eat identifier
	if (Lex.tokStr != ":") return (FuncArgAST*) error("Expected argument type.");
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
			if (Lex.tokStr != ",") return (FuncTypeAST*) error("Expected ')' or ',' in argument list.");
			Lex.gettok();
		}
	}

	Lex.gettok();	// eat ')'
	if (Lex.tokStr != "->") return (FuncTypeAST*) error("Invalid prototype : was expecting '->', not '" + Lex.tokStr + "'.");
	Lex.gettok();	// eat '->'
	if (Lex.tokStr == "{") {
		return new FuncTypeAST(args, &voidBaseType);
	}
	TypeAST *retType = ParseType();
	if (retType == 0) return 0;
	return new FuncTypeAST(args, retType);
}