#include "AST.h"

void BaseTypeAST::prettyprint(std::ostream &out) {
	if (BaseType == bt_void) out << "t_void";
	else if (BaseType == bt_int) out << "t_int";
	else if (BaseType == bt_float) out << "t_float";
	else out << "t_??";
}

void FuncArgAST::prettyprint(std::ostream &out) {
	out << "[" << Name << " as ";
	ArgType->prettyprint(out);
	out << "] ";
}

void FuncTypeAST::prettyprint(std::ostream &out) {
	out << "func(";;
	for (unsigned int i = 0; i < Args.size(); i++) {
		Args[i]->prettyprint(out);
	}
	out << " -> ";
	ReturnType->prettyprint(out);
	out << ")";
}

void BoolExprAST::prettyprint(std::ostream &out) {
	out << (Val ? "true" : "false");
}

void IntExprAST::prettyprint(std::ostream &out) {
	out << "int:" << Val;
}

void FloatExprAST::prettyprint(std::ostream &out) {
	out << "float:" << Val;
}

void VarExprAST::prettyprint(std::ostream &out) {
	out << "var_ref:" << Name;
}

void BinaryExprAST::prettyprint(std::ostream &out) {
	out << " (";
	LHS->prettyprint(out);
	out << ")" << Op << "(";
	RHS->prettyprint(out);
	out << ") ";
}

void CallExprAST::prettyprint(std::ostream &out) {
	out << "call[";
	Callee->prettyprint(out);
	out << "]:[";
	for (unsigned int i = 0; i < Args.size(); i++) {
		if (i >= 1) out << ",";
		Args[i]->prettyprint(out);
	}
	out << "]";
}

void BlockAST::prettyprint(std::ostream &out) {
	out << " block{";
	for (unsigned int i = 0; i < Instructions.size(); i++) {
		out << std::endl << "    ";
		Instructions[i]->prettyprint(out);
	}
	out << std::endl << "}" << std::endl;
}

void ReturnAST::prettyprint(std::ostream &out) {
	out << " return(";
	Val->prettyprint(out);
	out << ")";
}

void FuncExprAST::prettyprint(std::ostream &out) {
	out << " func ";
	Type->prettyprint(out);
	out << "  ";
	Code->prettyprint(out);
}

void ExternAST::prettyprint(std::ostream &out) {
	out << " <extern " << Symbol << " as ";
	if (Type == 0) {
		out << "[not resolved]";
	} else {
		Type->prettyprint(out);
	}
	out << "> ";
}

void DefAST::prettyprint(std::ostream &out) {
	out << " def:" << Name << " as:";
	if (Type == 0) {
		out << "[not resolved]";
	} else {
		Type->prettyprint(out);
	}
	out << " val:";
	Val->prettyprint(out);
}
