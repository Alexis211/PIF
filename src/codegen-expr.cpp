#include "AST-stmt.h"
#include "Generator.h"

using namespace llvm;
using namespace std;

void *ExprAST::error(const string &msg) {
	cerr << Tag.str() << " Error: " << msg << endl;
	return 0;
}


Value *BoolExprAST::Codegen() {
	return ConstantInt::get(getGlobalContext(), APInt(1, (Val ? 1 : 0), false));
}

Value *IntExprAST::Codegen() {
	return ConstantInt::get(getGlobalContext(), APInt(INTSIZE, Val, IType->Signed));
}

Value *FloatExprAST::Codegen() {
	return ConstantFP::get(getGlobalContext(), APFloat(Val));
}

Value *VarExprAST::Codegen() {
	if (Sym == 0) return (Value*) error(" (INTERNAL) Type checking didn't go here, that's bad.");
	if (IsGlobalConst) return Ctx->Gen->Builder.CreateLoad(Sym->llvmVal, "tmpload");
	return Sym->llvmVal;
}

Value *DerefExprAST::Codegen() {
	Value *v = Val->Codegen();
	if (v == 0) return 0;
	return Ctx->Gen->Builder.CreateLoad(v, "tmpderef");
}

Value *UnaryExprAST::Codegen() {
	Value *v = Expr->Codegen();

	if (Op == "-") {
		if (Expr->type(Ctx)->eq(FLOATTYPE)) {
			return Ctx->Gen->Builder.CreateFNeg(v, "negtmp");
		} else {
			return Ctx->Gen->Builder.CreateNeg(v, "negtmp");
		}
	} else if (Op == "!") {
		return Ctx->Gen->Builder.CreateNot(v, "nottmp");
	} else {
		return (Value*) error("(INTERNAL) Unknown unary operator '" + Op + "'.");
	}
}

Value *BinaryExprAST::Codegen() {
	IRBuilder<> &builder = Ctx->Gen->Builder;

	if (Op == "=") {
		Value *L = LHS->Codegen();
		Value *R = RHS->Codegen();
		if (L == 0 || R == 0) return 0;
		builder.CreateStore(R, L);
		return R;
	} else if (Op == "*" || Op == "+" || Op == "-" || Op == "/"  || Op == "%" ||
			Op == "<" || Op == ">" || Op == "==" || Op == "!=" || Op == "<=" || Op == ">=") {
		Value *L = LHS->Codegen();
		Value *R = RHS->Codegen();
		if (L == 0 || R == 0) return 0;

		IntTypeAST* li = dynamic_cast<IntTypeAST*>(LHS->type(Ctx)); 
		IntTypeAST* ri = dynamic_cast<IntTypeAST*>(RHS->type(Ctx)); 
		BaseTypeAST *lf = dynamic_cast<BaseTypeAST*>(LHS->type(Ctx));
		lf = (lf != 0 && lf->BaseType == bt_float ? lf : 0);
		BaseTypeAST *rf = dynamic_cast<BaseTypeAST*>(RHS->type(Ctx));
		rf = (rf != 0 && rf->BaseType == bt_float ? rf : 0);

		if (lf != 0 && rf != 0) {
			if (Op == "+") {
				return builder.CreateFAdd(L, R, "addtmp");
			} else if (Op == "-") {
				return builder.CreateFSub(L, R, "subtmp");
			} else if (Op == "*") {
				return builder.CreateFMul(L, R, "multmp");
			} else if (Op == "/") {
				return builder.CreateFDiv(L, R, "divtmp");
			} else if (Op == "%") {
				return builder.CreateFRem(L, R, "modtmp");
			} else if (Op == "<") {
				return builder.CreateFCmpOLT(L, R, "cmptmp");
			} else if (Op == ">") {
				return builder.CreateFCmpOGT(L, R, "cmptmp");
			} else if (Op == "<=") {
				return builder.CreateFCmpOLE(L, R, "cmptmp");
			} else if (Op == ">=") {
				return builder.CreateFCmpOGE(L, R, "cmptmp");
			} else if (Op == "==") {
				return builder.CreateFCmpOEQ(L, R, "cmptmp");
			} else if (Op == "!=") {
				return builder.CreateFCmpONE(L, R, "cmptmp");
			} else {
				return (Value*) error("Operator '" + Op +"' not defined for floats.");
			}
		} else if (li != 0 && ri != 0) {
			if (Op == "+") {
				return builder.CreateAdd(L, R, "addtmp");
			} else if (Op == "-") {
				return builder.CreateSub(L, R, "subtmp");
			} else if (Op == "*") {
				return builder.CreateMul(L, R, "multmp");
			} else if (Op == "/") {
				if (li->Signed) {
					return builder.CreateSDiv(L, R, "divtmp");				
				} else {
					return builder.CreateUDiv(L, R, "divtmp");				
				}
			} else if (Op == "%") {
				if (li->Signed) {
					return builder.CreateSRem(L, R, "divtmp");				
				} else {
					return builder.CreateURem(L, R, "divtmp");				
				}
			} else if (Op == "<") {
				if (li->Signed) {
					return builder.CreateICmpSLT(L, R, "cmptmp");
				} else {
					return builder.CreateICmpULT(L, R, "cmptmp");
				}
			} else if (Op == ">") {
				if (li->Signed) {
					return builder.CreateICmpSGT(L, R, "cmptmp");
				} else {
					return builder.CreateICmpUGT(L, R, "cmptmp");
				}
			} else if (Op == "<=") {
				if (li->Signed) {
					return builder.CreateICmpSLE(L, R, "cmptmp");
				} else {
					return builder.CreateICmpULE(L, R, "cmptmp");
				}
			} else if (Op == ">=") {
				if (li->Signed) {
					return builder.CreateICmpSGE(L, R, "cmptmp");
				} else {
					return builder.CreateICmpUGE(L, R, "cmptmp");
				}
			} else if (Op == "==") {
				return builder.CreateICmpEQ(L, R, "cmptmp");
			} else if (Op == "!=") {
				return builder.CreateICmpNE(L, R, "cmptmp");
			} else {
				return (Value*) error("Operator '" + Op +"' not defined for ints.");
			}
		} else {
			return (Value*) error("(INTERNAL) Cannot '" + Op + "' on something else than two ints or two floats.");
		}
	} else {
		return (Value*) error("(INTERNAL) Unimplemented operator");
	}
}

Value *DotMemberExprAST::Codegen() {
	if (Member == "") {
		return Obj->Codegen();
	} else {
		return (Value*) error("(INTERNAL) '.' action not implemented.");
	}
}

Value *CastExprAST::Codegen() {
	Value *v = Expr->Codegen();

	if (!NeedCast) {
		return v;
	} else {
		v = FType->castCodegen(v, Expr->type(Ctx), Ctx);
		if (v == 0) return (Value*) error("Bad cast.");
		return v;
	}
}

Value *CallExprAST::Codegen() {
	IRBuilder<> &builder = Ctx->Gen->Builder;

	FuncTypeAST *funcType = 0; 
	if (RefTypeAST *reft = dynamic_cast<RefTypeAST*>(Callee->type(Ctx))) {
		funcType = dynamic_cast<FuncTypeAST*>(reft->VType);
	}
	if (funcType == 0) {
		return (Value*) error(" (INTERNAL) Not a function type value is being called like a function.");
	}

	Value *calleev = Callee->Codegen();

	FunctionType *CalleeFT = 0; 
	if (PointerType *pt = dyn_cast<PointerType>(calleev->getType())) {
		CalleeFT = dyn_cast<FunctionType>(pt->getElementType());
	}
	if (CalleeFT == 0) return (Value*) error(" (INTERNAL) Trying to call something that is not a function.");

	if (funcType->Args.size() != Args.size())
		return (Value*) error(" (INTERNAL) Incorect argument count.");
	
	std::vector<Value*> ArgsV;
	for (unsigned i = 0; i < Args.size(); i++) {
		Value *av = Args[i]->Codegen();

		Type *avT = av->getType(), *expectedT = funcType->Args[i]->ArgType->getTy();

		if (avT != expectedT) {
			return (Value*) error(" (INTERNAL) Mismatch type for argument " + funcType->Args[i]->Name);
		}
		ArgsV.push_back(av);
		if (ArgsV.back() == 0) return 0;
	}

	if (CalleeFT->getReturnType() == Type::getVoidTy(getGlobalContext())) {
		builder.CreateCall(calleev, ArgsV);
		return 0;
	}
	
	return builder.CreateCall(calleev, ArgsV, "calltmp");
}

Value *ReturnAST::Codegen() {
	if (Val == 0) return Ctx->Gen->Builder.CreateRetVoid();

	Value *v = Val->Codegen();
	if (v == 0) return 0;
	return Ctx->Gen->Builder.CreateRet(v);
}

Value *IfThenElseAST::Codegen() {
	IRBuilder<> &builder = Ctx->Gen->Builder;

	Value *CondV = Cond->Codegen();
	if (CondV == 0) return 0;

	Function *fun = builder.GetInsertBlock()->getParent();
	BasicBlock *ThenBB = BasicBlock::Create(getGlobalContext(), "then", fun);
	BasicBlock *ElseBB = BasicBlock::Create(getGlobalContext(), "else");
	BasicBlock *MergeBB = BasicBlock::Create(getGlobalContext(), "ifcont");

	builder.CreateCondBr(CondV, ThenBB, ElseBB);

	builder.SetInsertPoint(ThenBB);
	Value *ThenV = TrueBr->Codegen();
	ThenBB = builder.GetInsertBlock();
	bool thenTerminated = (ThenBB->getTerminator() != 0);
	if (!thenTerminated) builder.CreateBr(MergeBB);

	fun->getBasicBlockList().push_back(ElseBB);
	builder.SetInsertPoint(ElseBB);
	Value *ElseV = (FalseBr != 0 ? FalseBr->Codegen() : 0);
	ElseBB = builder.GetInsertBlock();
	bool elseTerminated = (ElseBB->getTerminator() != 0);
	if (!elseTerminated) builder.CreateBr(MergeBB);

	if (thenTerminated && elseTerminated) return 0;

	fun->getBasicBlockList().push_back(MergeBB);
	builder.SetInsertPoint(MergeBB);
	if (ThenV != 0 && ElseV != 0) {
		if (ThenV->getType() != ElseV->getType()) {
			return (Value*)error("(INTERNAL) Not same types for then and else in if construct.");
		} else {
			PHINode *PN = builder.CreatePHI(ThenV->getType(), 2, "iftmp");
			PN->addIncoming(ThenV, ThenBB);
			PN->addIncoming(ElseV, ElseBB);
			return PN;
		}
	} else {
		return 0;
	}
}

Value *WhileAST::Codegen() {
	IRBuilder<> &builder = Ctx->Gen->Builder;

	Function *fun = builder.GetInsertBlock()->getParent();
	BasicBlock *CondBB = BasicBlock::Create(getGlobalContext(), "cond", fun);
	BasicBlock *DoBB = BasicBlock::Create(getGlobalContext(), "do", fun);
	BasicBlock *MergeBB = BasicBlock::Create(getGlobalContext(), "whilecont");

	builder.CreateBr(CondBB);

	builder.SetInsertPoint(CondBB);

	Value *CondV = Cond->Codegen();
	if (CondV == 0) return 0;

	if (IsUntil) {
		builder.CreateCondBr(CondV, MergeBB, DoBB);
	} else {
		builder.CreateCondBr(CondV, DoBB, MergeBB);
	}

	fun->getBasicBlockList().push_back(DoBB);
	builder.SetInsertPoint(DoBB);

	BasicBlock *prevCont = Ctx->More->ContinueTo, *prevBreak = Ctx->More->BreakTo;
	Ctx->More->ContinueTo = CondBB;
	Ctx->More->BreakTo = MergeBB;
	Inside->Codegen();
	Ctx->More->ContinueTo = prevCont;
	Ctx->More->BreakTo = prevBreak;

	DoBB = builder.GetInsertBlock();
	bool doTerminated = (DoBB->getTerminator() != 0);
	if (!doTerminated) {
		builder.CreateBr(CondBB);
	}
	fun->getBasicBlockList().push_back(MergeBB);
	builder.SetInsertPoint(MergeBB);
	return 0;
}

Value *BreakContAST::Codegen() {
	if (SType == bc_break) {
		if (Ctx->More->BreakTo == 0) return (Value*) error("Nowhere to break to.");
		Ctx->Gen->Builder.CreateBr(Ctx->More->BreakTo);
	} else {
		if (Ctx->More->ContinueTo == 0) return (Value*) error("Nowhere to break to.");
		Ctx->Gen->Builder.CreateBr(Ctx->More->ContinueTo);
	}
	return 0;
}

Value *BlockAST::Codegen() {
	for (unsigned i = 0; i < Instructions.size(); i++) {
		if (Ctx->Gen->Builder.GetInsertBlock()->getTerminator() != 0) {
			return (Value*)error(" You are writing code somewhere after your function has already returned.");
		}
		if (DefAST *d = dynamic_cast<DefAST*>(Instructions[i])) {
			if (VarDefAST *vd = dynamic_cast<VarDefAST*>(d)) {
				Value* val = vd->Val->Codegen();
				if (Ctx->NamedValues.back()->count(vd->Name) == 0) {
					 return (Value*)error(" (INTERNAL) Variable is declared, but not really.");
				}
				Symbol *s = Ctx->NamedValues.back()->find(vd->Name)->second;
				if (vd->Var) {
					s->llvmVal = Ctx->Gen->Builder.CreateAlloca(val->getType(), 0, vd->Name);
					Ctx->Gen->Builder.CreateStore(val, s->llvmVal);
				} else {
					s->llvmVal = val;
				}
			} else {
				return (Value*)error(" Non-toplevel functions not implemented.");
			}
		} else if (ExprStmtAST *e = dynamic_cast<ExprStmtAST*>(Instructions[i])) {
			e->Expr->Codegen();
		} else {
			return (Value*)error(" (INTERNAL) Something that should not be here is in a block.");
		}
	}
	return 0;
}


// Functions AST

Value *FuncExprAST::Codegen() {
	return (Value*) error("Lambda functions not implemented.");
}

Value *ExternAST::Codegen() {
	Module *mod = Ctx->Gen->TheModule;

	if (SType == 0) return (Value*) error(" (INTERNAL) Extern has no type.");

	if (FuncTypeAST *ftype = dynamic_cast<FuncTypeAST*>(SType)) {
		FunctionType *ft = dyn_cast<FunctionType>(SType->getTy());
		Function *f = Function::Create(ft, Function::ExternalLinkage, Symbol, mod);

		if (f->getName() != Symbol) {
			f->eraseFromParent();
			f = mod->getFunction(Symbol);
			if (!f->empty()) {
				return (Value*) error("Redefinition of extern function '" + Symbol + "'.");
			}
			if (f->arg_size() != ftype->Args.size()) {
				return (Value*) error("Redefinition of extern function '" + Symbol + "' with different argument count.");
			}
		}
		return f;
	}
	return (Value*) error("Extern symbols that are not functions are not supported yet.");
}
