#include "AST-stmt.h"

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

void DerefExprAST::prettyprint(std::ostream &out) {
	out << "*";
	Val->prettyprint(out);
}

void UnaryExprAST::prettyprint(std::ostream &out) {
	out << " " << Op << "(";
	Expr->prettyprint(out);
	out << ") ";
}

void BinaryExprAST::prettyprint(std::ostream &out) {
	out << " (";
	LHS->prettyprint(out);
	out << ")" << Op << "(";
	RHS->prettyprint(out);
	out << ") ";
}

void DotMemberExprAST::prettyprint(std::ostream &out) {
	out << "." << Member;
}

void CastExprAST::prettyprint(std::ostream &out) {
	Expr->prettyprint(out);
	out << " : " << FType->typeDescStr();
}

void CallExprAST::prettyprint(std::ostream &out) {
	out << "call[";
	Callee->prettyprint(out);
	out << "]:[";
	for (unsigned int i = 0; i < Args.size(); i++) {
		if (i >= 1) out << ",";
		if (Args[i] == 0) {
			out << "(invalid arg)";
		} else {
			Args[i]->prettyprint(out);
		}
	}
	out << "]";
}

void IfThenElseAST::prettyprint(std::ostream &out) {
	out << " if (";
	Cond->prettyprint(out);
	out << " then ";
	TrueBr->prettyprint(out);
	if (FalseBr != 0) {
		out << " else ";
		FalseBr->prettyprint(out);
	}
}

void WhileAST::prettyprint(std::ostream &out) {
	out << " while (";
	Cond->prettyprint(out);
	out << " do ";
	Inside->prettyprint(out);
}

void BreakContAST::prettyprint(std::ostream &out) {
	out << (SType == bc_break ? " BREAK " : " CONTINUE ");
}

void BlockAST::prettyprint(std::ostream &out) {
	out << " block{";
	for (unsigned int i = 0; i < Instructions.size(); i++) {
		out << std::endl << "    ";
		if (ExprStmtAST *e = dynamic_cast<ExprStmtAST*>(Instructions[i])) {
			e->Expr->prettyprint(out);
		} else if (DefAST *e = dynamic_cast<DefAST*>(Instructions[i])) {
			TypeAST *t = e->typeAtDef();
			out << "* def:" << e->Name << " : " << (t == 0 ? "[unknown type]" : t->typeDescStr()) << " = ";
			if (VarDefAST *vd = dynamic_cast<VarDefAST*>(e)) {
				vd->Val->prettyprint(out);
			} else {
				out << "??";
			}
		} else {
			out << "  ((bad stuff))  ";
		}
	}
	out << std::endl << "}" << std::endl;
}

void ReturnAST::prettyprint(std::ostream &out) {
	out << " return(";
	if (Val != 0) {
		Val->prettyprint(out);
	}
	out << ")";
}

void FuncExprAST::prettyprint(std::ostream &out) {
	out << " func " << FType->typeDescStr();
	out << "  ";
	Code->prettyprint(out);
}

void ExternAST::prettyprint(std::ostream &out) {
	out << " <extern " << Symbol << " as ";
	if (SType == 0) {
		out << "[not resolved]";
	} else {
		out << SType->typeDescStr();
	}
	out << "> ";
}

