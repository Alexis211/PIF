#ifndef DEF_ERROR_H
#define DEF_ERROR_H

#include <string>
#include <iostream>

#include "lexer/Lexer.h"

class PIFError {		// PIFError is considered an internal error.
	public:
	std::string message;
	PIFError *prev;

	PIFError(std::string m, PIFError *p) : message(m), prev(p) {};
	PIFError(std::string m) : message(m), prev(0) {};
	virtual void disp() {
		if (prev != 0) prev->disp();
		std::cerr << "[error] \t" << message << std::endl;
	}
};

class LangError : public PIFError {
	public:
	const FTag tag;

	LangError(const FTag &t, std::string m, PIFError *p) : PIFError(m, p), tag(t) {}
	LangError(const FTag &t, std::string m) : PIFError(m), tag(t) {}
	virtual void disp() {
		if (prev != 0) prev->disp();
		std::cerr << tag.str() << " \t" << message << std::endl;
	}

};

class InternalError : public PIFError {
	public:
	InternalError(std::string m, PIFError *p) : PIFError(m, p) {}
	InternalError(std::string m) : PIFError(m) {}
	virtual void disp() {
		if (prev != 0) prev->disp();
		std::cerr << "[internal error] \t" << message << std::endl;
	}
};

#endif

