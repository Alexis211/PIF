#include <cstdlib>
#include <cstdio>

#include "Lexer.h"

using namespace std;

Lexer::Lexer(std::string file, std::istream &in) : File(file), In(in) {
	LastChar = ' ';
	Line = 1;
	gettok();
}

Token Lexer::gettok() {
	tokStr = "";

	while (isspace(LastChar)) {
		if (LastChar == '\n') Line++;
		LastChar = In.get();
	}
	
	if (isalpha(LastChar) || LastChar == '_') {
		tokStr = LastChar;
		while (isalnum(LastChar = In.get()) || LastChar == '_')
			tokStr += LastChar;

		tok = tok_identifier;
		if (tokStr == "true") tok = tok_true;
		if (tokStr == "false") tok = tok_false;

		if (tokStr == "import") tok = tok_import;
		if (tokStr == "as") tok = tok_as;
		if (tokStr == "extern") tok = tok_extern;
		if (tokStr == "type") tok = tok_type;
		if (tokStr == "const") tok = tok_const;
		if (tokStr == "let") tok = tok_let;
		if (tokStr == "func") tok = tok_func;

		if (tokStr == "return") tok = tok_return;
		if (tokStr == "if") tok = tok_if;
		if (tokStr == "while") tok = tok_while;
	} else if (isdigit(LastChar) || LastChar == '.') {
		tokStr = "";
		bool is_float = false;
		do {
			tokStr += LastChar;
			if (LastChar == '.') is_float = true;
			LastChar = In.get();
		} while (isdigit(LastChar) || LastChar == '.');
		tokFloat = atof(tokStr.c_str());
		tokInt = atol(tokStr.c_str());
		tok = (is_float ? tok_float : tok_int);
	} else if (LastChar == '#') {
		do {
			LastChar = In.get();
		} while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

		if (LastChar != EOF) gettok();
	} else if (LastChar == EOF) {
		tok = tok_eof;
	} else {
		tok = tok_operator;
		tokStr = LastChar;
		while (1) {
			LastChar = In.get();
			if (LastChar == '+' || LastChar == '-' || LastChar == ':' || LastChar == '*' || LastChar == '/'
				|| LastChar == '>' || LastChar == '<') {
				tokStr += LastChar;
			} else {
				break;
			}
		}
	}
	return tok;
}
