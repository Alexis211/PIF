#include <sstream>

#include "AST-stmt.h"
#include "Generator.h"
#include "Package.h"

using namespace llvm;
using namespace std;

BaseTypeAST voidBaseType(bt_void), boolBaseType(bt_bool),
		floatBaseType(bt_float);
IntTypeAST u8iBaseType(8, false), s64iBaseType(64, true);



TypeAST *typeError(const FTag &tag, const string &msg) {
	cout << tag.str() << " Type Error: " << msg << endl;
	return 0;
}



TypeAST *ExprAST::type(Context *ctx) {
	if (ctx == 0) return (TypeAST*)error(" (INTERNAL ERROR) ExprAST::type called with no context.");

	if (Ctx == 0) Ctx = ctx;
	if (EType == NULL) {
		if (dep_loop) return typeError(Tag, " Expression dependency loop. That's bad, you know.");
		dep_loop = true;
		EType = this->getType();
	}
	return EType;
}

// String description for types

std::string PackageTypeAST::typeDescStr() {
	return "{" + Pkg->Name + "}";
}

std::string BaseTypeAST::typeDescStr() {
	if (BaseType == bt_void) return "void";
	if (BaseType == bt_bool) return "bool";
	if (BaseType == bt_float) return "float";
	return "[unknown base type]";
}

std::string IntTypeAST::typeDescStr() {
	std::stringstream o;
	o << (Signed ? "s" : "i") << Size << "i";
	return o.str();
}

std::string FuncTypeAST::typeDescStr() {
	std::string ret = "(";
	for (unsigned i = 0; i < Args.size(); i++) {
		if (i != 0) ret += ", ";
		ret += Args[i]->Name + " : " + Args[i]->ArgType->typeDescStr();
	}
	ret += ") -> " + ReturnType->typeDescStr();
	return ret;
}

std::string RefTypeAST::typeDescStr() {
	return "&" + VType->typeDescStr();
}

// Check types are equivalent

bool PackageTypeAST::eq(TypeAST *other) {
	return (this == other);
}

bool BaseTypeAST::eq(TypeAST *other) {
	BaseTypeAST *bt = dynamic_cast<BaseTypeAST*>(other);
	if (bt != 0)
		return bt->BaseType == BaseType;
	return false;
}

bool IntTypeAST::eq(TypeAST *other) {
	IntTypeAST *it = dynamic_cast<IntTypeAST*>(other);
	if (it != 0)
		return it->Size == Size && it->Signed == Signed;
	return false;
}

bool FuncTypeAST::eq(TypeAST *other) {
	FuncTypeAST *ft = dynamic_cast<FuncTypeAST*>(other);
	if (ft == 0) return false;
	if (!ft->ReturnType->eq(ReturnType)) return false;
	if (Args.size() != ft->Args.size()) return false;
	for (unsigned i = 0; i < Args.size(); i++) {
		if (!Args[i]->ArgType->eq(ft->Args[i]->ArgType)) return false;
	}
	return true;
}

bool RefTypeAST::eq(TypeAST *other) {
	if (this == other) return true;
	
	RefTypeAST *o = dynamic_cast<RefTypeAST*>(other);
	if (o == 0) return false;
	return VType->eq(o->VType);
}

// Type conversions

ExprAST *ExprAST::asTypeOrError(TypeAST *ty) {
	if (this != 0) {
		ExprAST *e = this->asType(ty);
		if (e != 0) {
			return e;
		}
	}
	cout << Tag.str() << " Type error: cannot use value of type '" << (EType != 0 ? EType->typeDescStr() : "???")
		<< "' as type '" << ty->typeDescStr() << "'." << endl;
	return 0;
}

ExprAST *ExprAST::asType(TypeAST *ty) {
	if (this == 0) return 0;
	if (this->Ctx == 0) return (ExprAST*) error(" (INTERNAL) Context not defined in ExprAST::asType.");

	TypeAST *thisty = this->type(Ctx);
	if (thisty->eq(ty)) return this;

	RefTypeAST *trty = dynamic_cast<RefTypeAST*>(thisty);
	if (trty != 0) {
		ExprAST *at = new DerefExprAST(Tag, this);
		if (at->type(Ctx)->eq(ty)) return at;
		at = at->asType(ty);
		if (at != 0) return at;
	}

	return 0;
}

ExprAST *IntExprAST::asType(TypeAST *ty) {
	if (this->Ctx == 0) return (ExprAST*) error(" (INTERNAL) Context not defined in IntExprAST::asType.");

	IntTypeAST *tyi = dynamic_cast<IntTypeAST*>(ty);
	if (tyi != 0) {
		//TODO someday : check size...
		IType = tyi;
		return this;
	}
	BaseTypeAST *tyb = dynamic_cast<BaseTypeAST*>(ty);
	if (tyb != 0 && tyb->BaseType == bt_float) {
		return new FloatExprAST(Tag, (FLOAT)Val);
	}
	return 0;
}

ExprAST *ExternAST::asType(TypeAST *ty) {
	if (this->Ctx == 0) return (ExprAST*) error(" (INTERNAL) Context not defined in ExternAST::asType.");

	RefTypeAST *rty = dynamic_cast<RefTypeAST*>(ty);
	if (rty == 0) return (ExprAST*) error(" Extern values must be of reference type.");

	if (SType == 0) {
		ExternAST *e = new ExternAST(Tag, rty->VType, Symbol);
		if (!e->type(Ctx)->eq(ty)) return (ExprAST*) error("Internal error type.cpp#15523425");
		return e;
	} else if (SType->eq(rty->VType)) {
		return this;
	} else {
		return 0;
	}
}

// Get TypeAST for expressions

TypeAST* IntExprAST::getType() {
	return IType;
}

TypeAST* BoolExprAST::getType() {
	return &boolBaseType;
}

TypeAST *FloatExprAST::getType() {
	return &floatBaseType;
}

TypeAST *VarExprAST::getType() {
	for (int i = Ctx->NamedValues.size() - 1; i >= 0; i--) {
		if (Ctx->NamedValues[i]->count(Name) != 0) {
			Sym = Ctx->NamedValues[i]->find(Name)->second;
			if (Sym->SType == 0) {
				return typeError(Tag, " Variable '" + Name + "' cannot be used here.");
			}
			if (i == 0) {
				if (VarDefAST *d = dynamic_cast<VarDefAST*>(Sym->Def)) {
					if (!d->Var) IsGlobalConst = true;
				}
			}
			return Sym->SType;
		}
	}
	if (Ctx->Pkg->Imports.count(Name) > 0) {
		return Ctx->Pkg->Imports[Name]->PkgType;
	}
	return typeError(Tag, " Variable '" + Name + "' not found.");
}

TypeAST *DerefExprAST::getType() {
	TypeAST *t = Val->type(Ctx);
	if (t == 0) return 0;
	RefTypeAST *rt = dynamic_cast<RefTypeAST*>(t);
	if (rt == 0) {
		return typeError(Tag, " Dereferencing something that was not a reference...");
	}
	if (dynamic_cast<FuncTypeAST*>(rt->VType) != 0) {
		return typeError(Tag, " Cannot dereference a function pointer. You call it as-is.");
	}
	return rt->VType;
}

TypeAST *UnaryExprAST::getType() {
	TypeAST *inType = Expr->type(Ctx);
	if (inType == 0) return 0;

	if (Op == "!") {
		if (inType->eq(&boolBaseType)) {
			return inType;
		} else {
			return typeError(Tag, " Unary negation '!' only works with bools.");
		}
	} else if (Op == "-") {
		if (inType->eq(&floatBaseType)) return inType;
		if (dynamic_cast<IntTypeAST*>(inType) != 0) return inType;
		return typeError(Tag, " Unary negation '-' only works with ints or floats.");
	} else {
		return typeError(Tag, " Unknown unary operator '" + Op + "'.");
	}
}

TypeAST *BinaryExprAST::getType() {
	if (Op == "=") {
		TypeAST *lt = LHS->type(Ctx);
		if (lt == 0) return 0;
		TypeAST *rt = RHS->type(Ctx);
		if (rt == 0) return 0;

		RefTypeAST *ltr = dynamic_cast<RefTypeAST*>(lt);
		if (ltr == 0) return typeError(Tag, "Cannot affect to non-reference value.");

		if (rt->eq(ltr->VType)) {
			return rt;
		} else {
			RHS = RHS->asTypeOrError(ltr->VType);
			if (RHS != 0) {
				if (!ltr->VType->eq(RHS->type(Ctx))) return typeError(Tag, "Internal error 234666");
				return RHS->type(Ctx);
			} else {
				return typeError(Tag, "Cannot affect '" + rt->typeDescStr() + "' to '" + lt->typeDescStr() + "'.");
			}
		}

	} else if (Op == "*" || Op == "+" || Op == "-" || Op == "/"  || Op == "%" ||
			Op == "<" || Op == ">" || Op == "==" || Op == "!=" || Op == "<=" || Op == ">=") {

		bool retBool = (Op == "<" || Op == ">" || Op == "==" || Op == "!=" || Op == "<=" || Op == ">=");
		TypeAST *retT = (retBool ? &boolBaseType : 0);

		TypeAST *lt = LHS->type(Ctx);
		while (lt != 0 && dynamic_cast<RefTypeAST*>(lt) != 0) {
			LHS = new DerefExprAST(Tag, LHS);
			lt = LHS->type(Ctx);
		}
		TypeAST *rt = RHS->type(Ctx);
		while (rt != 0 && dynamic_cast<RefTypeAST*>(rt) != 0) {
			RHS = new DerefExprAST(Tag, RHS);
			rt = RHS->type(Ctx);
		}

		if (lt == 0 || rt == 0) return 0;

		IntTypeAST *li = dynamic_cast<IntTypeAST*>(lt);
		IntTypeAST *ri = dynamic_cast<IntTypeAST*>(rt);
		BaseTypeAST *lf = dynamic_cast<BaseTypeAST*>(lt);
		lf = (lf != 0 && lf->BaseType == bt_float ? lf : 0);
		BaseTypeAST *rf = dynamic_cast<BaseTypeAST*>(rt);
		rf = (rf != 0 && rf->BaseType == bt_float ? rf : 0);
		
		if ((lf != 0 && rf != 0) || (lf != 0 && ri != 0) || (li != 0 && rf != 0)) {
			if (lf == 0) {
				LHS = LHS->asTypeOrError(rf);
				if (LHS == 0) return typeError(Tag, " cannot compute '" + Op + "' operation (LHS to float fail).");
				lf = rf;
			}
			if (rf == 0) {
				RHS = RHS->asTypeOrError(lf);
				if (RHS == 0) return typeError(Tag, " cannot compute '" + Op + "' operation (RHS to float fail).");
			}
			if (retT == 0) retT = lf;
		} else if (li != 0 && ri != 0) {
			if (!li->eq(ri)) {
				if (li->Size >= ri->Size) {
					if (ri->Signed) li->Signed = true;
					RHS = new CastExprAST(Tag, RHS, li);
					if (retT == 0) retT = li;
				} else {
					if (li->Signed) ri->Signed = true;
					LHS = new CastExprAST(Tag, LHS, ri);
					if (retT == 0) retT = ri;
				}
			} else {
				if (retT == 0) retT = li;
			}
		} else {
			return typeError(Tag, " operator '" + Op + "' only defined on integers and floats.");
		}
		return retT;
	} else {
		return typeError(Tag, " unknown or unimplemented operator '" + Op + "'.");
	}
}

TypeAST *DotMemberExprAST::getType() {
	TypeAST *inType = Obj->type(Ctx);
	if (inType == 0) return 0;
	if (PackageTypeAST *pt = dynamic_cast<PackageTypeAST*>(inType)) {
		Obj = new VarExprAST(Tag, Member);
		TypeAST *retType = (Obj->type(&pt->Pkg->Ctx));
		if (retType == 0)
			return typeError(Tag, " no such member '" + Member + "' in package '" + pt->Pkg->Name + "'.");
		Member = "";
		return retType;
	} else {
		return typeError(Tag, " accessing a member using '.' is only possible with packages.");
	}
}

TypeAST *CastExprAST::getType() {
	TypeAST *fromT = Expr->type(Ctx);
	if (fromT == 0) return 0;

	if (fromT->eq(FType)) {
		cout << Tag.str() << " Notice: unnecessary cast." << endl;
		NeedCast = false;
		return FType;
	}

	ExprAST *contAsType = Expr->asType(FType);
	if (contAsType != 0) {
		Expr = contAsType;
		NeedCast = false;
		return FType;
	}

	while (1) {
		
		// Test INT<->FLOAT casts
		IntTypeAST *fromi = dynamic_cast<IntTypeAST*>(fromT);
		BaseTypeAST *fromf = dynamic_cast<BaseTypeAST*>(fromT);
		fromf = (fromf != 0 && fromf->BaseType == bt_float ? fromf : 0);

		IntTypeAST *toi = dynamic_cast<IntTypeAST*>(FType);
		BaseTypeAST *tof = dynamic_cast<BaseTypeAST*>(FType);
		tof = (tof != 0 && tof->BaseType == bt_float ? tof : 0);
		if ((fromi != 0 || fromf != 0) && (toi != 0 || tof != 0)) {
			return FType;
		}

		// Nothing found, try dereferencing and looping again
		if (dynamic_cast<RefTypeAST*>(fromT) != 0) {
			Expr = new DerefExprAST(Tag, Expr);
			fromT = Expr->type(Ctx);
			if (fromT == 0) {
				error("(INTERNAL) Dereferencing fail.");
				break;
			}
		} else {
			break;
		}
	}

	// End, really nothing found.
	cerr << Tag.str() << " Error: impossible cast from '" <<
		fromT->typeDescStr() << "' to '" << FType->typeDescStr() << "'." << endl;
	return 0;
}

TypeAST *CallExprAST::getType() {
	TypeAST *t = Callee->type(Ctx);

	FuncTypeAST *funct = 0; 
	if (RefTypeAST *reft = dynamic_cast<RefTypeAST*>(t)) {
		funct = dynamic_cast<FuncTypeAST*>(reft->VType);
	}

	if (funct != 0) {
		// Check argument types
		if (Args.size() != funct->Args.size()) {
			return typeError(Tag, 
				"Wrong number of arguments for call to function of type " + funct->typeDescStr() + ".");
		}
		for (unsigned i = 0; i < Args.size(); i++) {
			TypeAST *t = Args[i]->type(Ctx);
			if (t == 0) return 0;
			if (!t->eq(funct->Args[i]->ArgType)) {
				Args[i] = Args[i]->asTypeOrError(funct->Args[i]->ArgType);
				if (Args[i] == 0) {
					return typeError(Tag, "Wrong type for argument " + funct->Args[i]->Name + ".");
				}
			}
		}
		return funct->ReturnType;
	} else {
		cout << "Type of callee: " << t->typeDescStr() << endl;
		return typeError(Tag, "Calling something that is not a function.");
	}
}

TypeAST *IfThenElseAST::getType() {
	TypeAST *condType = Cond->type(Ctx);
	if (condType == 0) return 0;
	if (!condType->eq(&boolBaseType)) return typeError(Tag, "Condition in 'if' must be boolean.");
	
	TypeAST *thenType = TrueBr->type(Ctx);
	if (thenType == 0) return 0;
	if (FalseBr == 0) {
		if (!thenType->eq(&voidBaseType)) return typeError(Tag, "Expression in 'then' must be void when no 'else'.");
		return thenType;
	}

	TypeAST *elseType = FalseBr->type(Ctx);
	if (elseType == 0) return 0;

	if (!thenType->eq(elseType)) {
		FalseBr = FalseBr->asTypeOrError(thenType);
		if (FalseBr == 0) {
			return typeError(Tag, "Expression in 'then' and 'else' must be of same type.");
		}
	}
	return thenType;
}

TypeAST *WhileAST::getType() {
	TypeAST *condType = Cond->type(Ctx);
	if (condType == 0) return 0;
	if (!condType->eq(&boolBaseType)) return typeError(Tag, "Condition in while/until must be of type bool.");

	TypeAST *contType = Inside->type(Ctx);
	if (contType == 0) return 0;

	return &voidBaseType;
}

TypeAST *BreakContAST::getType() {
	// OOH THIS TIME WE HAVE NOTHING TO DO !! THAT'S GOOD !!
	return &voidBaseType;
}

TypeAST *BlockAST::getType() {
	if (!OwnContext) {
		Ctx = new Context(*Ctx);
		Ctx->NamedValues.push_back(new map<string, Symbol*>());
	}
	for (unsigned i = 0; i < Instructions.size(); i++) {
		if (!Instructions[i]->typeCheck(Ctx)) {
			if (DefAST *d = dynamic_cast<DefAST*>(Instructions[i])) {
				return typeError(Tag, "Type checking error in definition of '" + d->Name + "'.");
			} else {
				return typeError(Tag, "Type checking error somewhere in this block.");
			}
		}
		if (DefAST *d = dynamic_cast<DefAST*>(Instructions[i])) {
			Symbol *s = new Symbol(d);
			s->SType = d->typeAtDef();
			if (s->SType == 0) {
				cerr << "probably a bug (BlockAST::getType())" << endl;
				return 0;
			}
			Ctx->NamedValues.back()->insert(pair<string, Symbol*>(d->Name, s));
		}
	}
	OwnContext = true;
	return &voidBaseType;
}

TypeAST *ReturnAST::getType() {
	TypeAST *retT = Ctx->More->FuncRetType;
	if (Val != 0) {
		TypeAST *exprT = Val->type(Ctx);
		if (exprT == 0) return 0;
		if (exprT->eq(retT)) {
			return &voidBaseType;
		} else {
			Val = Val->asType(retT);
			if (Val != 0) {
				if (Val->type(Ctx)->eq(retT)) {
					return &voidBaseType;
				} else {
					return typeError(Tag, "Internal error #4525426");
				}
			}
		}
	} else {
		if (retT->eq(&voidBaseType)) {
			return &voidBaseType;
		}
	}
	return typeError(Tag, "Return statement does not return correct type value.");
}

TypeAST *ExternAST::getType() {
	if (SType == 0) return &voidBaseType;
	return RefTypeAST::Get(SType);
}

TypeAST *FuncExprAST::getType() {
	if (!OwnContext) {			// register arguments in new context
		Ctx = new Context(*Ctx);
		OwnContext = true;

		Ctx->More = new MoreContext(FType->ReturnType);
		map<string, Symbol*> *m = new map<string, Symbol*>();
		for (unsigned i = 0; i < FType->Args.size(); i++) {
			m->insert(pair<string, Symbol*>(FType->Args[i]->Name, new Symbol(
					FType->Args[i]->ArgType, 0
				)));
		}
		Ctx->NamedValues.push_back(m);
	}
	if (Code->type(Ctx) == 0) return 0;
	return RefTypeAST::Get(FType);
}


// ============= Type checking in definitions ==========

bool VarDefAST::typeCheck(Context *ctx) {
	TypeAST* t2 = Val->type(ctx);
	if (t2 == 0) {
		cerr << Tag.str() << " Cannot determine type of value for '" << Name << "'." << endl;
		return 0;
	}
	if (VType == 0) {
		VType = t2;
	} else {
		if (!VType->eq(t2)) {
			Val = Val->asTypeOrError(VType);
			if (Val == 0) return false;
		}
	}
	return true;
}

bool FuncDefAST::typeCheck(Context *ctx) {
	return Val->type(ctx) != 0;
}

bool ExternFuncDefAST::typeCheck(Context *ctx) {
	if (EType == 0) return false;
	if (Val->type(ctx) == 0) return false;
	Val = dynamic_cast<ExternAST*>(Val->asTypeOrError(RefTypeAST::Get(EType)));
	return (Val != 0);
}

bool ExprStmtAST::typeCheck(Context *ctx) {
	return (Expr->type(ctx) != 0);
}

// *****

TypeAST *VarDefAST::typeAtDef() {
	if (VType == 0) return 0;
	if (Var) return RefTypeAST::Get(VType);
	return VType;
}

TypeAST *FuncDefAST::typeAtDef() {
	return RefTypeAST::Get(Val->FType);
}

TypeAST *ExternFuncDefAST::typeAtDef() {
	return RefTypeAST::Get(EType);
}
