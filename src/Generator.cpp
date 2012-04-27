#include "Generator.h"

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

bool Generator::build(Package *pkg) {
	/*cerr << "GENERATOR SAYS: Wait a minute. This isn't ready yet." << endl;
	return false;*/

	string prefix = pkg->SymbolPrefix;

	// Setup package symbol table
	for (map<string, Symbol*>::iterator it = pkg->Symbols.begin(); it != pkg->Symbols.end(); it++) {

		cerr << "setup: " << it->first << " : " << it->second->SType->typeDescStr() << endl;

		DefAST *d = it->second->Def;

		if (VarDefAST *vd = dynamic_cast<VarDefAST*>(d)) {
			GlobalVariable *pos = new GlobalVariable(*TheModule, vd->VType->getTy(), false, GlobalValue::ExternalLinkage, UndefValue::get(vd->VType->getTy()), prefix + d->Name);
			it->second->llvmVal = pos;
		} else if (FuncDefAST *fd = dynamic_cast<FuncDefAST*>(d)) {
			string sym_name = prefix + fd->Name;
			FunctionType *ft = dyn_cast<FunctionType>(fd->Val->FType->getTy());
			if (ft == 0) {
				cerr << d->Tag.str() << " Error: function does not have function type." << endl;
				return false;
			}
			Function *f = Function::Create(ft, Function::ExternalLinkage, sym_name, TheModule);

			if (f->getName() != sym_name) {
				cerr << d->Tag.str() << " Error: Redefinition of function " << fd->Name << endl;
				return false;
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
	cerr << "Symbols ok" << endl;

	// Generate code for functions
	for (map<string, Symbol*>::iterator it = pkg->Symbols.begin(); it != pkg->Symbols.end(); it++) {
		DefAST *d = it->second->Def;
		if (FuncDefAST *fd = dynamic_cast<FuncDefAST*>(d)) {
			string sym_name = pkg->SymbolPrefix + fd->Name;
			Function *f = TheModule->getFunction(sym_name);
			if (f != it->second->llvmVal) {
				cerr << d->Tag.str() << " Internal error n°RN#45556, sorry." << endl;
				return false;
			}

			Context *fctx = fd->Val->Ctx;

			unsigned i = 0;
			for (Function::arg_iterator ai = f->arg_begin(); i != fd->Val->FType->Args.size(); i++, ai++) {
				auto s = fctx->NamedValues.back()->find(fd->Val->FType->Args[i]->Name);
				if (s == fctx->NamedValues.back()->end()) {
					cerr << " (INTERNAL ERROR) Function argument name mismatch." << endl;
				} else {
					s->second->llvmVal = ai;
				}
			}

			BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "entry", f);
			Builder.SetInsertPoint(BB);

			if (fd->Name == "_init") {
				pkg->InitFunction = f;
				for (unsigned i = 0; i < pkg->VarDefs.size(); i++) {
					VarDefAST *vd = dynamic_cast<VarDefAST*>(pkg->VarDefs[i]->Def);
					if (vd == 0) {
						cerr << " (INTERNAL ERROR) global variable is not a variable." << endl;
						return false;
					}
					Value *v = vd->Val->Codegen();
					Builder.CreateStore(v, pkg->VarDefs[i]->llvmVal);
				}
			}

			fd->Val->Code->Codegen();

			BB = Builder.GetInsertBlock();
			if (BB->getTerminator() == 0) {
				if (f->getReturnType() == Type::getVoidTy(getGlobalContext())) {
					Builder.CreateRetVoid();
				} else {
					fd->Val->error("Function '" + it->first + "' lacks a return statement.");
					return false;
				}
			}

			f->dump();
			if (verifyFunction(*f)) {
				fd->Val->error("Error in function '" + it->first + "'...");
				fd->Val->prettyprint(cerr);
			}
			FPM.run(*f);
		}
	}

	if (pkg->InitFunction == 0) {
		cerr << "Internal error #1512351, sorry." << endl;
		return false;
	}

	return true;
}

void Generator::init(Package *package) {
	if (package->Complete == false) {
		cerr << "Internal error." << endl;
		return;
	}
	if (package->InitFunction == 0) return;

	CallsInMain.push_back(package->InitFunction);

	/*void *FPtr = ExecEng->getPointerToFunction(package->InitFunction);
	void (*FP)() = (void (*)())(intptr_t)FPtr;
	FP();*/
}

void Generator::main(Package *package) {
	if (package->Complete == false) {
		cerr << "Internal error." << endl;
		return;
	}

	if (package->Symbols.count("_main") == 0) {
		cerr << "Error : no _main function defined." << endl;
	}
	Function *f = dyn_cast<Function>(package->Symbols["_main"]->llvmVal);
	if (f == 0) {
		cerr << "Error: invalid _main function." << endl;
	}

	CallsInMain.push_back(f);

	vector<Type*> _args;
	FunctionType *main_ft = FunctionType::get(f->getReturnType(), _args, false);
	Function *main_f = Function::Create(main_ft, Function::ExternalLinkage, "main", TheModule);
	if (main_f->getName() != "main") {
		cerr << "(INTERNAL ERROR?) Something is probably wrong with main function, sorry." << endl;
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

	main_f->dump();
	verifyFunction(*main_f);
	FPM.run(*main_f);


	void *FPtr = ExecEng->getPointerToFunction(main_f);
	void (*FP)() = (void (*)())(intptr_t)FPtr;
	FP();
}
