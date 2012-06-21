#include <sstream>

#include "../ast/stmt.h"
#include "../Package.h"

#include "../util.h"
#include "../error.h"

using namespace llvm;
using namespace std;



TypeAST *ExprAST::type(Context *ctx) {
	if (ctx == 0) throw new InternalError("ExprAST::type called with no context.");

	if (Ctx == 0) Ctx = ctx;
	if (EType == NULL) {
		if (dep_loop) Tag.Throw("Type error: expression dependency loop. That's bad, you know.");
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

// Type dereferencing possibilities

bool TypeAST::canDeref() {
	return false;
}

bool RefTypeAST::canDeref() {
	return (dynamic_cast<FuncTypeAST*>(VType) == 0);
}

// Type conversions

ExprAST *ExprAST::asTypeOrError(TypeAST *ty) {
	if (this != 0) {
		ExprAST *e = this->asType(ty);
		if (e != 0) {
			return e;
		}
	}
	Tag.Throw("Type error: cannot use value of type '" + (EType != 0 ? EType->typeDescStr() : "???")
		+ "' as type '" + ty->typeDescStr());
	return 0;
}

ExprAST *ExprAST::asType(TypeAST *ty) {
	if (this == 0) return 0;
	if (this->Ctx == 0) throw new InternalError("Context not defined in ExprAST::asType.");

	TypeAST *thisty = this->type(Ctx);
	if (thisty == ty) return this;

	if (thisty->canDeref()) {
		ExprAST *at = new DerefExprAST(Tag, this);
		if (at->type(Ctx) == ty) return at;
		at = at->asType(ty);
		if (at != 0) return at;
	}

	return 0;
}

ExprAST *IntExprAST::asType(TypeAST *ty) {
	if (this->Ctx == 0) throw new InternalError("Context not defined in IntExprAST::asType.");

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
	if (this->Ctx == 0) throw new InternalError("Context not defined in ExternAST::asType.");

	RefTypeAST *rty = dynamic_cast<RefTypeAST*>(ty);
	if (rty == 0) throw new InternalError("Extern values must be of reference type.");

	if (SType == 0) {
		ExternAST *e = new ExternAST(Tag, rty->VType, Symbol);
		if (e->type(Ctx) != ty) throw new InternalError("Internal error type.cpp#15523425");
		return e;
	} else if (SType == rty->VType) {
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
	return BOOLTYPE;
}

TypeAST *FloatExprAST::getType() {
	return FLOATTYPE;
}

TypeAST *VarExprAST::getType() {
	for (int i = Ctx->NamedValues.size() - 1; i >= 0; i--) {
		if (Ctx->NamedValues[i]->count(Name) != 0) {
			Sym = Ctx->NamedValues[i]->find(Name)->second;
			if (Sym->SType == 0) {
				Tag.Throw("Scope error: variable '" + Name + "' cannot be used here.");
			}
			if (i == 0) {
				if (VarDefAST *d = dynamic_cast<VarDefAST*>(Sym->Def)) {
					if (!d->Var) IsGlobalConst = true;
				}
			}
			return Sym->SType;
		}
	}
	Package *import = Ctx->Pkg->getImport(Name);
	if (import != 0) {
		return PackageTypeAST::Get(import);
	}
	Tag.Throw("Scope error: variable '" + Name + "' not found.");
	return 0;
}

TypeAST *DerefExprAST::getType() {
	TypeAST *t = Val->type(Ctx);
	if (t == 0) return 0;
	RefTypeAST *rt = dynamic_cast<RefTypeAST*>(t);
	if (rt == 0) {
		Tag.Throw("Dereferencing something that was not a reference...");
	}
	if (dynamic_cast<FuncTypeAST*>(rt->VType) != 0) {
		Tag.Throw("Cannot dereference a function pointer. You call it as-is.");
	}
	return rt->VType;
}

TypeAST *UnaryExprAST::getType() {
	TypeAST *inType = Expr->type(Ctx);
	if (inType == 0) return 0;

	if (Op == "!") {
		if (inType == BOOLTYPE) {
			return inType;
		} else {
			Tag.Throw("Unary negation '!' only works with bools.");
		}
	} else if (Op == "-") {
		if (inType == FLOATTYPE) return inType;
		if (dynamic_cast<IntTypeAST*>(inType) != 0) return inType;
		Tag.Throw("Unary negation '-' only works with ints or floats.");
	} else {
		Tag.Throw("Unknown unary operator '" + Op + "'.");
	}
	return 0;
}

TypeAST *BinaryExprAST::getType() {
	if (Op == "=") {
		TypeAST *lt = LHS->type(Ctx);
		if (lt == 0) return 0;
		TypeAST *rt = RHS->type(Ctx);
		if (rt == 0) return 0;

		RefTypeAST *ltr = dynamic_cast<RefTypeAST*>(lt);
		if (ltr == 0) Tag.Throw("Cannot affect to non-reference value.");

		if (rt == ltr->VType) {
			return rt;
		} else {
			RHS = RHS->asTypeOrError(ltr->VType);
			if (RHS != 0) {
				if (ltr->VType != RHS->type(Ctx)) Tag.Throw("Internal error 234666");
				return RHS->type(Ctx);
			} else {
				Tag.Throw("Cannot affect '" + rt->typeDescStr() + "' to '" + lt->typeDescStr() + "'.");
			}
		}

	} else if (Op == "*" || Op == "+" || Op == "-" || Op == "/"  || Op == "%" ||
			Op == "<" || Op == ">" || Op == "==" || Op == "!=" || Op == "<=" || Op == ">=") {

		bool retBool = (Op == "<" || Op == ">" || Op == "==" || Op == "!=" || Op == "<=" || Op == ">=");
		TypeAST *retT = (retBool ? BOOLTYPE : 0);

		TypeAST *lt = LHS->type(Ctx);
		while (lt != 0 && lt->canDeref()) {
			LHS = new DerefExprAST(Tag, LHS);
			lt = LHS->type(Ctx);
		}
		TypeAST *rt = RHS->type(Ctx);
		while (rt != 0 && rt->canDeref()) {
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
				if (LHS == 0) Tag.Throw("cannot compute '" + Op + "' operation (LHS to float fail).");
				lf = rf;
			}
			if (rf == 0) {
				RHS = RHS->asTypeOrError(lf);
				if (RHS == 0) Tag.Throw("cannot compute '" + Op + "' operation (RHS to float fail).");
			}
			if (retT == 0) retT = lf;
		} else if (li != 0 && ri != 0) {
			if (li != ri) {
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
			Tag.Throw("Operator '" + Op + "' only defined on integers and floats.");
		}
		return retT;
	} else {
		Tag.Throw("Unknown or unimplemented operator '" + Op + "'.");
	}
}

TypeAST *DotMemberExprAST::getType() {
	TypeAST *inType = Obj->type(Ctx);
	if (inType == 0) return 0;
	if (PackageTypeAST *pt = dynamic_cast<PackageTypeAST*>(inType)) {
		Obj = new VarExprAST(Tag, Member);
		TypeAST *retType = (Obj->type(&pt->Pkg->Ctx));
		if (retType == 0)
			Tag.Throw( "no such member '" + Member + "' in package '" + pt->Pkg->Name + "'.");
		Member = "";
		return retType;
	} else {
		Tag.Throw("accessing a member using '.' is only possible with packages.");
	}
}

TypeAST *CastExprAST::getType() {
	TypeAST *fromT = Expr->type(Ctx);
	if (fromT == 0) return 0;

	if (fromT == FType) {
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
		if (fromT->canDeref()) {
			Expr = new DerefExprAST(Tag, Expr);
			fromT = Expr->type(Ctx);
			if (fromT == 0) {
				throw new InternalError("Dereferencing fail.");
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
			Tag.Throw(
				"Wrong number of arguments for call to function of type " + funct->typeDescStr() + ".");
		}
		for (unsigned i = 0; i < Args.size(); i++) {
			TypeAST *t = Args[i]->type(Ctx);
			if (t == 0) return 0;
			if (t != funct->Args[i]->ArgType) {
				Args[i] = Args[i]->asTypeOrError(funct->Args[i]->ArgType);
				if (Args[i] == 0) {
					Tag.Throw("Wrong type for argument " + funct->Args[i]->Name + ".");
				}
			}
		}
		return funct->ReturnType;
	} else {
		cout << "Type of callee: " << t->typeDescStr() << endl;
		Tag.Throw("Calling something that is not a function.");
	}
}

TypeAST *IfThenElseAST::getType() {
	TypeAST *condType = Cond->type(Ctx);
	if (condType == 0) return 0;
	if (condType != BOOLTYPE) Tag.Throw("Condition in 'if' must be boolean.");
	
	TypeAST *thenType = TrueBr->type(Ctx);
	if (thenType == 0) return 0;
	if (FalseBr == 0) {
		if (thenType != VOIDTYPE) Tag.Throw("Expression in 'then' must be void when no 'else'.");
		return thenType;
	}

	TypeAST *elseType = FalseBr->type(Ctx);
	if (elseType == 0) return 0;

	if (thenType != elseType) {
		FalseBr = FalseBr->asTypeOrError(thenType);
		if (FalseBr == 0) {
			Tag.Throw("Expression in 'then' and 'else' must be of same type.");
		}
	}
	return thenType;
}

TypeAST *WhileAST::getType() {
	TypeAST *condType = Cond->type(Ctx);
	if (condType == 0) return 0;
	if (condType != BOOLTYPE) Tag.Throw("Condition in while/until must be of type bool.");

	TypeAST *contType = Inside->type(Ctx);
	if (contType == 0) return 0;

	return VOIDTYPE;
}

TypeAST *BreakContAST::getType() {
	// OOH THIS TIME WE HAVE NOTHING TO DO !! THAT'S GOOD !!
	return VOIDTYPE;
}

TypeAST *BlockAST::getType() {
	if (!OwnContext) {
		Ctx = new Context(*Ctx);
		Ctx->NamedValues.push_back(new map<string, Symbol*>());
	}
	for (unsigned i = 0; i < Instructions.size(); i++) {
		try {
			Instructions[i]->typeCheck(Ctx);
		} catch (PIFError *e) {
			if (DefAST *d = dynamic_cast<DefAST*>(Instructions[i])) {
				Tag.Throw("Type checking error in definition of '" + d->Name + "'.", e);
			} else {
				Tag.Throw("Type checking error somewhere in this block.", e);
			}
		}
		if (DefAST *d = dynamic_cast<DefAST*>(Instructions[i])) {
			Symbol *s = new Symbol(d);
			s->SType = d->typeAtDef();
			if (s->SType == 0) {
				throw InternalError("probably a bug (BlockAST::getType())");
			}
			Ctx->NamedValues.back()->insert(pair<string, Symbol*>(d->Name, s));
		}
	}
	OwnContext = true;
	return VOIDTYPE;
}

TypeAST *ReturnAST::getType() {
	TypeAST *retT = Ctx->More->FuncRetType;
	if (Val != 0) {
		TypeAST *exprT = Val->type(Ctx);
		if (exprT == 0) return 0;
		if (exprT == retT) {
			return VOIDTYPE;
		} else {
			Val = Val->asType(retT);
			if (Val != 0) {
				if (Val->type(Ctx) == retT) {
					return VOIDTYPE;
				} else {
					Tag.Throw("Internal error #4525426");
				}
			}
		}
	} else {
		if (retT == VOIDTYPE) {
			return VOIDTYPE;
		}
	}
	Tag.Throw("Return statement does not return correct type value.");
}

TypeAST *ExternAST::getType() {
	if (SType == 0) return VOIDTYPE;
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

void VarDefAST::typeCheck(Context *ctx) {
	DBGC(cerr << "TC:\t"; Val->prettyprint(cerr); cerr << endl);

	TypeAST* t2;
	try {
		t2 = Val->type(ctx);
	} catch (PIFError *e) {
		Tag.Throw("Cannot determine type of value for '" + Name + "'.", e);
	}
	if (VType == 0) {
		VType = t2;
	} else {
		if (VType != t2) {
			try {
				Val = Val->asTypeOrError(VType);
				if (Val == 0) Tag.Throw("Type check error for '" + Name + "'.");
			} catch (PIFError* e) {
				 Tag.Throw("Type check error for '" + Name + "'.", e);
			}
		}
	}
}

void FuncDefAST::typeCheck(Context *ctx) {
	DBGC(cerr << "TC:\t"; Val->prettyprint(cerr); cerr << endl);

	if (Val->type(ctx) == 0) Tag.Throw("Type check error for '" + Name + "'.");
}

void ExternFuncDefAST::typeCheck(Context *ctx) {
	DBGC(cerr << "TC:\t"; Val->prettyprint(cerr); cerr << endl);

	try {
		if (EType == 0) Tag.Throw("Extern with no type : '" + Name + "'.");
		if (Val->type(ctx) == 0) Tag.Throw("Cannot evaluate type of :'" + Name + "'.");
		Val = dynamic_cast<ExternAST*>(Val->asTypeOrError(RefTypeAST::Get(EType)));
		if (Val == 0) Tag.Throw("Type check error for '" + Name + "'.");
	} catch (PIFError *e) {
		Tag.Throw("Type check error for '" + Name + "'.", e);
	}
}

void ExprStmtAST::typeCheck(Context *ctx) {
	if (Expr->type(ctx) == 0) Tag.Throw("Type check error.");
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
