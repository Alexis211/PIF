#include "AST-stmt.h"
#include "Generator.h"
#include "Package.h"

using namespace llvm;
using namespace std;

// Type manager

PackageTypeAST *PackageTypeAST::Get(Package *pkg) {
	return pkg->PkgType;
}

map<TypeAST*, RefTypeAST*> refTypes;
RefTypeAST *RefTypeAST::Get(TypeAST *type) {
	if (refTypes.count(type) == 0) {
		refTypes[type] = new RefTypeAST(type);
	}
	return refTypes[type];
}

map<BaseTypeE, BaseTypeAST*> baseTypes;
BaseTypeAST *BaseTypeAST::Get(BaseTypeE basetype) {
	if (baseTypes.count(basetype) == 0) {
		baseTypes[basetype] = new BaseTypeAST(basetype);
	}
	return baseTypes[basetype];
}

map<int, IntTypeAST*> intTypes;
IntTypeAST *IntTypeAST::Get(short size, bool sign) {
	int id = size * 2 + (sign ? 1 : 0);
	if (intTypes.count(id) == 0) {
		intTypes[id] = new IntTypeAST(size, sign);
	}
	return intTypes[id];
}

map<string, FuncTypeAST*> funcTypes;
FuncTypeAST *FuncTypeAST::Get(const std::vector<FuncArgAST*> &args, TypeAST* retType) {
	string id = "(";
	for (unsigned i = 0; i < args.size(); i++) {
		if (i > 0) id += ",";
		id += args[i]->Name + ":" + args[i]->ArgType->typeDescStr();
	}
	id += ")->" + retType->typeDescStr();

	if (funcTypes.count(id) == 0) {
		funcTypes[id] = new FuncTypeAST(args, retType);
	} else {
		for (unsigned i = 0; i < args.size(); i++) {
			delete args[i];	// THIS IS DANGEROUS BUT IT IS.
		}
	}
	return funcTypes[id];
}

// Get LLVM type from TypeAST 

Type *PackageTypeAST::getTy() {
	cerr << "(INTERNAL ERROR) Package types are not llvm types." << endl;
	return 0;
}

Type *BaseTypeAST::getTy() {
	if (BaseType == bt_bool) return Type::getInt1Ty(getGlobalContext());
	if (BaseType == bt_float) return Type::getDoubleTy(getGlobalContext());
	return Type::getVoidTy(getGlobalContext());
}

Type *IntTypeAST::getTy() {
	// TODO : return different type for unsigned values ?
	return Type::getIntNTy(getGlobalContext(), Size);
}

Type *FuncTypeAST::getTy() {
	vector<Type*> ArgsTy;
	for (unsigned i = 0; i < Args.size(); i++) {
		ArgsTy.push_back(Args[i]->ArgType->getTy());
	}
	return FunctionType::get(ReturnType->getTy(), ArgsTy, false);
}

Type *RefTypeAST::getTy() {
	return PointerType::get(VType->getTy(), 0);
}


// Cast operations

Value *TypeAST::castCodegen(llvm::Value *v, TypeAST *origType, Context *ctx) {
	if (this->eq(origType)) return v;
	cerr << " (INTERNAL ERROR) Unimplemented cast from '" << origType->typeDescStr() << "' to '"
		<< this->typeDescStr() << "'." << endl;
	return 0;
}

Value *BaseTypeAST::castCodegen(llvm::Value *v, TypeAST *origType, Context *ctx) {
	if (BaseType != bt_float) return 0;

	IntTypeAST *fromi = dynamic_cast<IntTypeAST*>(origType);
	if (fromi == 0) return 0;

	if (fromi->Signed) {
		return ctx->Gen->Builder.CreateSIToFP(v, this->getTy(), "sitofptmp");
	} else {
		return ctx->Gen->Builder.CreateUIToFP(v, this->getTy(), "uitofptmp");
	}
}

Value *IntTypeAST::castCodegen(llvm::Value *v, TypeAST *origType, Context *ctx) {
	if (dynamic_cast<IntTypeAST*>(origType) != 0) {
		return ctx->Gen->Builder.CreateIntCast(v, this->getTy(), Signed, "itoitmp");
	} else if (BaseTypeAST *frombt = dynamic_cast<BaseTypeAST*>(origType)) {
		if (frombt->BaseType != bt_float) return 0;
		if (Signed) {
			return ctx->Gen->Builder.CreateFPToSI(v, this->getTy(), "fptositmp");
		} else {
			return ctx->Gen->Builder.CreateFPToUI(v, this->getTy(), "fptouitmp");
		}
	} else {
		return 0;
	}
}
