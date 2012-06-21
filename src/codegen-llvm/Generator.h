#ifndef DEF_GENERATOR_H
#define DEF_GENERATOR_H

#include "../Package.h"

#include <llvm/PassManager.h>

#include <llvm/Target/TargetData.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/LLVMContext.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Support/TargetSelect.h>

#include <vector>

class Generator {
	public:
	llvm::Module *TheModule;
	llvm::IRBuilder<> Builder;
	llvm::FunctionPassManager FPM;

	llvm::ExecutionEngine *ExecEng;

	std::vector<llvm::Function*> CallsInMain;

	Generator();

	void build(Package *package);
	void init(Package *package);
	void main(Package *package);
};


#endif

