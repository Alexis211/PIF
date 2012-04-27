#include "AST.h"
#include "Package.h"

BaseTypeAST voidBaseType(bt_void), boolBaseType(bt_bool),
		intBaseType(bt_int), floatBaseType(bt_float);

using namespace llvm;
using namespace std;

// Get LLVM type from TypeAST 

Type *BaseTypeAST::getTy() {
	if (BaseType == bt_bool) return Type::getInt1Ty(getGlobalContext());
	if (BaseType == bt_int) return Type::getIntNTy(getGlobalContext(), INTSIZE);
	if (BaseType == bt_float) return Type::getDoubleTy(getGlobalContext());
	return Type::getVoidTy(getGlobalContext());
}

Type *FuncTypeAST::getTy() {
	vector<Type*> ArgsTy;
	for (unsigned i = 0; i < Args.size(); i++) {
		ArgsTy.push_back(Args[i]->ArgType->getTy());
	}
	return FunctionType::get(ReturnType->getTy(), ArgsTy, false);
}

// Get TypeAST for expressions

TypeAST *errorTA(const FTag &tag, const string &msg) {
	cout << tag.str() << " Type Error: " << msg << endl;
	return 0;
}

TypeAST* IntExprAST::type(Context *ctx) {
	return &intBaseType;
}

TypeAST* BoolExprAST::type(Context *ctx) {
	return &boolBaseType;
}

TypeAST *FloatExprAST::type(Context *ctx) {
	return &floatBaseType;
}

TypeAST *VarExprAST::type(Context *ctx) {
	DefAST *S = ctx->Pkg->Symbols[Name];
	if (S == 0) return 0;
	return S->Type;
}

TypeAST *BinaryExprAST::type(Context *ctx) {
	//TODO : what type is it, really ?
	return RHS->type(ctx);
}

TypeAST *CallExprAST::type(Context *ctx) {
	TypeAST *t = Callee->type(ctx);
	if (FuncTypeAST *funct = dynamic_cast<FuncTypeAST*>(t)) {
		return funct->ReturnType;
	} else {
		return errorTA(Tag, "Calling something that is not a function.");
	}
}

TypeAST *BlockAST::type(Context *ctx) {
	return &voidBaseType;
}

TypeAST *ReturnAST::type(Context *ctx) {
	return Val->type(ctx);
}

TypeAST *ExternAST::type(Context *ctx) {
	return Type;
}

TypeAST *FuncExprAST::type(Context *ctx) {
	return Type;
}
