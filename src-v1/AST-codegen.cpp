#include "AST.h"
#include "Generator.h"

using namespace llvm;
using namespace std;

Value *errorV(const FTag &tag, const string &msg) {
	cerr << tag.str() << " Error: " << msg << endl;
	return 0;
}


Value *BoolExprAST::Codegen(Context *ctx) {
	return ConstantInt::get(getGlobalContext(), APInt(1, (Val ? 1 : 0), false));
}

Value *IntExprAST::Codegen(Context *ctx) {
	return ConstantInt::get(getGlobalContext(), APInt(INTSIZE, Val, true));
}

Value *FloatExprAST::Codegen(Context *ctx) {
	return ConstantFP::get(getGlobalContext(), APFloat(Val));
}

Value *VarExprAST::Codegen(Context *ctx) {
	if (ctx->NamedValues.count(Name) == 0) {
		return errorV(Tag, "Unknown variable '" + Name + "'.");
	} else {
		return ctx->NamedValues[Name];
	}
}

Value *BinaryExprAST::Codegen(Context *ctx) {
	IRBuilder<> &builder = ctx->Gen->Builder;

	if (Op == ".") {
		// SPECIAL CASE!! TODO
		return errorV(Tag, "'.' operator not yet implemented.");
	} else if (Op == "=") {
		// SPECIAL CASE!! TODO
		return errorV(Tag, "affectation operator '=' and mutable variables not yet implemented.");
	} else {
		Value *L = LHS->Codegen(ctx);
		Value *R = RHS->Codegen(ctx);
		if (L == 0 || R == 0) return 0;

		bool li = L->getType()->isIntegerTy();
		bool ri = R->getType()->isIntegerTy();
		bool lf = L->getType()->isFloatingPointTy();
		bool rf = R->getType()->isFloatingPointTy();

		if ((lf && rf) || (lf && ri) || (li && rf)) {
			Value *LF = (lf ? L : (builder.CreateSIToFP(L, floatBaseType.getTy(), "lftmp")));
			Value *RF = (rf ? R : (builder.CreateSIToFP(R, floatBaseType.getTy(), "rftmp")));
			if (Op == "+") {
				return builder.CreateFAdd(LF, RF, "addtmp");
			} else if (Op == "-") {
				return builder.CreateFSub(LF, RF, "subtmp");
			} else if (Op == "*") {
				return builder.CreateFMul(LF, RF, "multmp");
			} else if (Op == "/") {
				return builder.CreateFDiv(LF, RF, "divtmp");
			} else if (Op == "<") {
				return builder.CreateFCmpULT(LF, RF, "cmptmp");
			} else if (Op == ">") {
				return builder.CreateFCmpUGT(LF, RF, "cmptmp");
			} else {
				return errorV(Tag, "Operator '" + Op +"' not defined for floats.");
			}
		} else if (li || ri) {
			if (Op == "+") {
				return builder.CreateAdd(L, R, "addtmp");
			} else if (Op == "-") {
				return builder.CreateSub(L, R, "subtmp");
			} else if (Op == "*") {
				return builder.CreateMul(L, R, "multmp");
			} else if (Op == "/") {
				return builder.CreateSDiv(L, R, "divtmp");
			} else if (Op == "<") {
				return builder.CreateICmpSLT(L, R, "cmptmp");
			} else if (Op == ">") {
				return builder.CreateICmpSGT(L, R, "cmptmp");
			} else {
				return errorV(Tag, "Operator '" + Op +"' not defined for ints.");
			}
		} else {
			return errorV(Tag, "Cannot '" + Op + "' on something else than ints or floats.");
		}
	}
	return errorV(Tag, "This should not happen.");
}

Value *CallExprAST::Codegen(Context *ctx) {
	IRBuilder<> &builder = ctx->Gen->Builder;

	FuncTypeAST *funcType = dynamic_cast<FuncTypeAST*>(Callee->type(ctx));
	if (funcType == 0) return errorV(Tag, "Not a function type value is being called like a function.");

	Function *CalleeF = dyn_cast<Function>(Callee->Codegen(ctx));
	if (CalleeF == 0) return errorV(Tag, "Trying to call something that is not a function.");

	if (CalleeF->arg_size() != Args.size())
		return errorV(Tag, "Incorect argument count.");
	
	std::vector<Value*> ArgsV;
	for (unsigned i = 0; i < Args.size(); i++) {
		Value *av = Args[i]->Codegen(ctx);

		Type *avT = av->getType(), *expectedT = funcType->Args[i]->ArgType->getTy();

		if (avT->isIntegerTy() && expectedT->isFloatingPointTy()) {
			av = builder.CreateSIToFP(av, floatBaseType.getTy(), "argtmp");
		} else if (avT->isFloatingPointTy() && expectedT->isIntegerTy()) {
			cerr << Tag.str() << " Warning: Devaluating float argument for '"
				<< funcType->Args[i]->Name << "' to int." << endl;
			av = builder.CreateFPToSI(av, intBaseType.getTy(), "argtmp");
		} else if (avT != expectedT) {
			return errorV(Tag, "Mismatch type for argument " + funcType->Args[i]->Name);
		}
		ArgsV.push_back(av);
		if (ArgsV.back() == 0) return 0;
	}

	if (CalleeF->getReturnType() == Type::getVoidTy(getGlobalContext())) {
		builder.CreateCall(CalleeF, ArgsV);
		return 0;
	}
	
	return builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

Value *ReturnAST::Codegen(Context *ctx) {
	Value *v = Val->Codegen(ctx);
	if (v == 0) return 0;
	return ctx->Gen->Builder.CreateRet(v);
}

Value *BlockAST::Codegen(Context *ctx) {
	IRBuilder<> &builder = ctx->Gen->Builder;

	BasicBlock *BB = BasicBlock::Create(getGlobalContext());
	builder.SetInsertPoint(BB);
	for (unsigned i = 0; i < Instructions.size(); i++) {
		Instructions[i]->Codegen(ctx);
	}
	return BB;
}

void BlockAST::FctEntryCodegen(Context *ctx) {
	IRBuilder<> &builder = ctx->Gen->Builder;

	BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "entry", ctx->Fun);
	builder.SetInsertPoint(bb);

	for (unsigned i = 0; i < Instructions.size(); i++) {
		Instructions[i]->Codegen(ctx);
	}
}


// Functions AST

Value *FuncExprAST::Codegen(Context *ctx) {
	return errorV(Tag, "Lambda functions not implemented.");
}

Value *ExternAST::Codegen(Context *ctx) {
	Module *mod = ctx->Gen->TheModule;

	if (Type == 0) return errorV(Tag, "Extern has no type.");

	if (FuncTypeAST *ftype = dynamic_cast<FuncTypeAST*>(Type)) {
		FunctionType *ft = dyn_cast<FunctionType>(Type->getTy());
		Function *f = Function::Create(ft, Function::ExternalLinkage, Symbol, mod);

		if (f->getName() != Symbol) {
			f->eraseFromParent();
			f = mod->getFunction(Symbol);
			if (!f->empty()) {
				return errorV(Tag, "Redefinition of extern function '" + Symbol + "'.");
			}
			if (f->arg_size() != ftype->Args.size()) {
				return errorV(Tag, "Redefinition of extern function '" + Symbol + "' with different argument count.");
			}
		}
		return f;
	}
	return errorV(Tag, "Extern symbols that are not functions are not supported yet.");
}
