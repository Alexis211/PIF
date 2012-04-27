#ifndef DEF_LEXER_H
#define DEF_LEXER_H

#include <string>
#include <iostream>
#include <sstream>
#include <set>

#include "config.h"

enum Token {
	tok_eof,
	tok_identifier,
	tok_float,
	tok_int,
	tok_operator,

	tok_true,
	tok_false,

	tok_import,
	tok_as,
	tok_extern,
	tok_type,
	tok_const,
	tok_let,
	tok_var,
	tok_func,
	
	tok_return,
	tok_if,
	tok_then,
	tok_else,
	tok_while,
	tok_until,
	tok_do,
	tok_break,
	tok_continue,
};

class FTag {			// Identifies a position in a file
	private:
	std::string File, Tok;
	int Line;
	public:
	FTag(std::string file, std::string tok, int line) : File(file), Tok(tok), Line(line) {}
	FTag() : File("_"), Tok(""), Line(-1) {}
	std::string str() const {
		std::stringstream out;
		out << "[" << File << ":" << Line << " near '" << Tok << "']";
		return out.str();
	}
};

class Lexer {
	private:
	std::string File;
	std::istream &In;

	std::set<std::string> MultiCharOps;

	int LastChar;
	int Line;

	public:
	Lexer(std::string file, std::istream &in);

	Token gettok();

	std::string tokStr;
	FLOAT tokFloat;
	INT tokInt;
	Token tok;

	FTag tag() { return FTag(File, tokStr, Line); }
};

#endif
