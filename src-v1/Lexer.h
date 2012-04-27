#ifndef DEF_LEXER_H
#define DEF_LEXER_H

#include <string>
#include <iostream>
#include <sstream>

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
	tok_func,
	
	tok_return,
	tok_if,
	tok_while,
};

class FTag {			// Identifies a position in a file
	private:
	std::string File;
	int Line;
	public:
	FTag(std::string file, int line) : File(file), Line(line) {}
	std::string str() const {
		std::stringstream out;
		out << "[" << File << ":" << Line << "]";
		return out.str();
	}
};

class Lexer {
	private:
	std::string File;
	std::istream &In;

	int LastChar;
	int Line;

	public:
	Lexer(std::string file, std::istream &in);

	Token gettok();

	std::string tokStr;
	FLOAT tokFloat;
	INT tokInt;
	Token tok;

	FTag tag() { return FTag(File, Line); }
};

#endif
