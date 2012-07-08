#include "Generator.h"
#include "../util.h"
#include "../error.h"

using namespace llvm;
using namespace std;

Generator::Generator() :
	TheModule(new Module("PIF", getGlobalContext())),
	Builder(getGlobalContext()),
	FPM(TheModule)
	{
		
	InitializeNativeTarget();
	ExecEng = EngineBuilder(TheModule).create();

	FPM.add(new TargetData(*ExecEng->getTargetData()));
	FPM.add(createBasicAliasAnalysisPass());
	FPM.add(createPromoteMemoryToRegisterPass());
	FPM.add(createInstructionCombiningPass());
	FPM.add(createReassociatePass());
	FPM.add(createGVNPass());
	FPM.add(createCFGSimplificationPass());
	FPM.doInitialization();
}

void Generator::build(Package *pkg) {
	string prefix = pkg->SymbolPrefix;

	// Setup package symbol table
	for (map<string, Symbol*>::iterator it = pkg->Symbols.begin(); it != pkg->Symbols.end(); it++) {

		DBGP(cerr << "setup: " << it->first << " : " << it->second->SType->typeDescStr() << endl)

		DefAST *d = it->second->Def;

		if (VarDefAST *vd = dynamic_cast<VarDefAST*>(d)) {

			DBGC(cerr << " -> " << (vd->Var ? "var" : "const" ) << " = "; vd->Val->prettyprint(cerr))

			GlobalVariable *pos = new GlobalVariable(*TheModule, vd->VType->getTy(), false, GlobalValue::ExternalLinkage, UndefValue::get(vd->VType->getTy()), prefix + d->Name);
			it->second->llvmVal = pos;
		} else if (FuncDefAST *fd = dynamic_cast<FuncDefAST*>(d)) {

			DBGC(cerr << " -> func = "; fd->Val->prettyprint(cerr))

			string sym_name = prefix + fd->Name;
			FunctionType *ft = dyn_cast<FunctionType>(fd->Val->FType->getTy());
			if (ft == 0) {
				d->Tag.Throw(" Error: function does not have function type.");
			}
			Function *f = Function::Create(ft, Function::ExternalLinkage, sym_name, TheModule);

			if (f->getName() != sym_name) {
				d->Tag.Throw(" Error: Redefinition of function " + fd->Name);
			}

			// set names for arguments
			unsigned i = 0;
			for (Function::arg_iterator ai = f->arg_begin(); i != fd->Val->FType->Args.size(); i++, ai++) {
				ai->setName(fd->Val->FType->Args[i]->Name);
			}
			it->second->llvmVal = f;
		} else if (ExternFuncDefAST *ed = dynamic_cast<ExternFuncDefAST*>(d)) {
			it->second->llvmVal = ed->Val->Codegen();
		}
	}

	DBGP(cerr << "Symbols ok" << endl)

	// Generate code for functions
	for (map<string, Symbol*>::iterator it = pkg->Symbols.begin(); it != pkg->Symbols.end(); it++) {
		DefAST *d = it->second->Def;
		if (FuncDefAST *fd = dynamic_cast<FuncDefAST*>(d)) {
			string sym_name = pkg->SymbolPrefix + fd->Name;
			Function *f = TheModule->getFunction(sym_name);
			if (f != it->second->llvmVal) {
				d->Tag.Throw(" Internal error n°RN#45556, sorry.");
			}

			Context *fctx = fd->Val->Ctx;

			unsigned i = 0;
			for (Function::arg_iterator ai = f->arg_begin(); i != fd->Val->FType->Args.size(); i++, ai++) {
				auto s = fctx->NamedValues.back()->find(fd->Val->FType->Args[i]->Name);
				if (s == fctx->NamedValues.back()->end()) {
					throw new InternalError("Function argument name mismatch.");
				} else {
					s->second->llvmVal = ai;
				}
			}

			BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "entry", f);
			Builder.SetInsertPoint(BB);

			if (fd->Name == "_init") {
				pkg->InitFunction = f;
				for (unsigned i = 0; i < pkg->SymbolDefOrder.size(); i++) {
					Symbol *s = pkg->SymbolDefOrder[i];
					VarDefAST *vd = dynamic_cast<VarDefAST*>(s->Def);
					if (vd == 0) continue;
					Value *v = vd->Val->Codegen();
					Builder.CreateStore(v, s->llvmVal);
				}
			}

			fd->Val->Code->Codegen();

			BB = Builder.GetInsertBlock();
			if (BB->getTerminator() == 0) {
				if (f->getReturnType() == Type::getVoidTy(getGlobalContext())) {
					Builder.CreateRetVoid();
				} else {
					fd->Val->Tag.Throw("Function '" + it->first + "' lacks a return statement.");
				}
			}

			DBGC(f->dump())

			if (verifyFunction(*f)) {
				fd->Val->Tag.Throw("Error in function '" + it->first + "'...");
			}
			FPM.run(*f);
		}
	}

	if (pkg->InitFunction == 0) {
		throw new InternalError("Internal error #1512351, sorry.");
	}
}

void Generator::init(Package *package) {
	if (package->Complete == false) {
		throw new InternalError("Internal error #1513542, sorry.");
	}
	if (package->InitFunction == 0) return;

	CallsInMain.push_back(package->InitFunction);
}

// === Helper function for main ===
void Generator::main(Package *package) {
	if (package->Complete == false) {
		throw new InternalError("Internal error #2652462, sorry.");
	}

	if (package->Symbols.count("_main") == 0) {
		throw new PIFError("no _main function defined in '" + package->Name + "'.");
	}
	Function *f = dyn_cast<Function>(package->Symbols["_main"]->llvmVal);
	if (f == 0) {
		throw new PIFError("invalid _main function in '" + package->Name + "'.");
	}

	CallsInMain.push_back(f);

	vector<Type*> _args;
	FunctionType *main_ft = FunctionType::get(f->getReturnType(), _args, false);
	Function *main_f = Function::Create(main_ft, Function::ExternalLinkage, "main", TheModule);
	if (main_f->getName() != "main") {
		throw new InternalError("Something is probably wrong with main function, sorry.");
	}
	
	Value *retval;
	BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "entry", main_f);
	Builder.SetInsertPoint(BB);
	std::vector<Value*> _args_v;
	for (unsigned i = 0; i < CallsInMain.size(); i++) {
		if (CallsInMain[i]->getReturnType() == Type::getVoidTy(getGlobalContext())) {
			retval = 0;
			Builder.CreateCall(CallsInMain[i], _args_v);
		} else {
			retval = Builder.CreateCall(CallsInMain[i], _args_v);
		}
	}
	if (retval != 0) {
		Builder.CreateRet(retval);
	} else {
		Builder.CreateRetVoid();
	}

	DBGC(main_f->dump())

	if (verifyFunction(*main_f)) {
		throw new InternalError("Incorrect main function...");
	}
	FPM.run(*main_f);

	// Call that function
	void *FPtr = ExecEng->getPointerToFunction(main_f);
	void (*FP)() = (void (*)())(intptr_t)FPtr;
	FP();

	CallsInMain.clear();
}
