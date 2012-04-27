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
	FPM.add(createInstructionCombiningPass());
	FPM.add(createReassociatePass());
	FPM.add(createGVNPass());
	FPM.add(createCFGSimplificationPass());
	FPM.doInitialization();
}

bool Generator::build(Package *package) {
	Context c(package, this);
	// generate headers for functions
	for (map<string, DefAST*>::iterator it = package->Symbols.begin(); it != package->Symbols.end(); it++) {
		DefAST *d = it->second;
		TypeAST *t = d->Type;
		if (t == 0) t = d->Type = d->Val->type(&c);
		if (t == 0) continue;

		if (ExternAST *extrn = dynamic_cast<ExternAST*>(d->Val)) {
			c.NamedValues[d->Name] = extrn->Codegen(&c);
		} else if (dynamic_cast<FuncExprAST*>(d->Val)) {
			if (FuncTypeAST *ftast = dynamic_cast<FuncTypeAST*>(t)) {
				string sym_name = package->SymbolPrefix + d->Name;
				FunctionType *ft = dyn_cast<FunctionType>(ftast->getTy());
				if (ft == 0) {
					cerr << d->Tag.str() << " Error: function does not have function type." << endl;
					return false;
				}
				Function *f = Function::Create(ft, Function::ExternalLinkage, sym_name, TheModule);

				if (f->getName() != sym_name) {
					f->eraseFromParent();
					f = TheModule->getFunction(sym_name);
					if (!f->empty()) {
						cerr << d->Tag.str() << " Error: Redefinition of function " << sym_name << endl;
						return false;
					}
					if (f->arg_size() != ftast->Args.size()) {
						cerr << d->Tag.str() << " Error: Redefinition of function " << sym_name << " with different argument count" << endl;
						return false;
					}
				}
				// set names for arguments
				unsigned i = 0;
				for (Function::arg_iterator ai = f->arg_begin(); i != ftast->Args.size(); i++, ai++) {
					ai->setName(ftast->Args[i]->Name);
				}
				c.NamedValues[d->Name] = f;
			} else {
				cerr << d->Tag.str() << " Error: Function body doesn not have function type." << endl;
			}
		} else {
			c.NamedValues[d->Name] = Builder.CreateAlloca(t->getTy(), 0, package->SymbolPrefix + d->Name);
			c.NamedValues[d->Name]->dump();
		}
	}

	// generate code for functions
	for (map<string, DefAST*>::iterator it = package->Symbols.begin(); it != package->Symbols.end(); it++) {
		DefAST *d = it->second;
		TypeAST *t = d->Type;
		if (t == 0) continue;
		if (FuncTypeAST *ftast = dynamic_cast<FuncTypeAST*>(t)) {
			if (dynamic_cast<ExternAST*>(d->Val)) {
				// Case of an extern definition is already ok.
				continue;
			}

			Context fctx(c);

			string sym_name = package->SymbolPrefix + d->Name;
			Function *f = TheModule->getFunction(sym_name);
			if (f != c.NamedValues[d->Name]) {
				cerr << d->Tag.str() << " Internal error n°RN#45555, sorry." << endl;
				return false;
			}

			fctx.Fun = f;

			// get arguments into context
			unsigned i = 0;
			for (Function::arg_iterator ai = f->arg_begin(); i != ftast->Args.size(); i++, ai++) {
				fctx.NamedValues[ftast->Args[i]->Name] = ai;
			}

			if (FuncExprAST *fast = dynamic_cast<FuncExprAST*>(d->Val)) {
				fast->Code->FctEntryCodegen(&fctx);
			} else {
				cerr << d->Tag.str() << " Error: function does not have function body." << endl;
				return false;
			}

			verifyFunction(*f);
			f->dump();
			FPM.run(*f);		// Run optimization passes
		}
	}

	// generate code for init function
	string init_fct_name = package->SymbolPrefix + "__init";
	FunctionType *init_fct_type = FunctionType::get(Type::getVoidTy(getGlobalContext()), false);
	Function *init_fct = Function::Create(init_fct_type, Function::ExternalLinkage, init_fct_name, TheModule);

	if (init_fct->getName() != init_fct_name) {
		init_fct->eraseFromParent();
		init_fct = TheModule->getFunction(init_fct_name);
		if (!init_fct->empty()) {
			cerr << "Error: multiple definition of __init function for package." << endl;
			return false;
		} else if (init_fct->arg_size() != 0) {
			cerr << "Error: multiple definition of __init function for package." << endl;
			return false;
		}
	}

	BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "entry", init_fct);
	Builder.SetInsertPoint(bb);
	for (unsigned i = 0; i < package->InitCode.size(); i++) {
		package->InitCode[i]->Codegen(&c);
	}
	Builder.CreateRetVoid();

	verifyFunction (*init_fct);
	init_fct->dump();
	FPM.run(*init_fct);			// OPTIMIZE!!!!!
	
	package->InitFunction = init_fct;

	return true;
}

void Generator::init(Package *package) {
	if (package->Complete == false) return;

	void *FPtr = ExecEng->getPointerToFunction(package->InitFunction);
	void (*FP)() = (void (*)())(intptr_t)FPtr;
	FP();
}
