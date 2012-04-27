#ifndef DEF_AST_TYPE_H
#define DEF_AST_TYPE_H

#include "config.h"

#include "Lexer.h"

#include <string>
#include <map>
#include <vector>
#include <iostream>

#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>

class Context;
class Package;

// TypeAST - Base class for all type nodes
class TypeAST {
	public:
	virtual ~TypeAST() {}
	virtual llvm::Type *getTy() = 0;
	virtual bool eq(TypeAST *other);

	virtual std::string typeDescStr() = 0;

	virtual llvm::Value *castCodegen(llvm::Value *v, TypeAST *origType, Context *ctx);
};

// PackageTypeAST - We need packages to be considered as types in some cases
class PackageTypeAST : public TypeAST {
	friend class DotMemberExprAST;
	friend class Package;

	Package *Pkg;

	PackageTypeAST (Package *pkg) : Pkg(pkg) {}
	public:
	static PackageTypeAST *Get(Package *pkg);

	virtual llvm::Type *getTy();
	virtual std::string typeDescStr();
};

// BaseTypeAST - Class for types like int or float
enum BaseTypeE {
	bt_void,
	bt_bool,
	bt_float,
};
class BaseTypeAST : public TypeAST {
	friend class BinaryExprAST;
	friend class IntExprAST;
	friend class IntTypeAST;
	friend class CastExprAST;

	BaseTypeE BaseType;
	BaseTypeAST(BaseTypeE basetype) : BaseType(basetype) {}
	public:
	static BaseTypeAST* Get(BaseTypeE basetype);

	virtual llvm::Type *getTy();

	virtual std::string typeDescStr();

	virtual llvm::Value *castCodegen(llvm::Value *v, TypeAST *origType, Context *ctx);
};
#define VOIDTYPE (BaseTypeAST::Get(bt_void))
#define BOOLTYPE (BaseTypeAST::Get(bt_bool))
#define FLOATTYPE (BaseTypeAST::Get(bt_float))

class IntTypeAST : public TypeAST {
	friend class BaseTypeAST;
	friend class BinaryExprAST;
	friend class IntExprAST;

	short Size;
	bool Signed;

	IntTypeAST(short size, bool sign) : Size(size), Signed(sign) {}
	public:
	static IntTypeAST *Get(short size, bool sign);

	virtual llvm::Type *getTy();

	virtual std::string typeDescStr();

	virtual llvm::Value *castCodegen(llvm::Value *v, TypeAST *origType, Context *ctx);
};
#define INTTYPE (IntTypeAST::Get(INTSIZE, true))


// FuncArgAST - Class for function arguments in definitions
class FuncArgAST {
	friend class FuncTypeAST;
	friend class Generator;
	friend class CallExprAST;
	friend class FuncExprAST;

	std::string Name;
	TypeAST* ArgType;
	public:
	FuncArgAST(std::string name, TypeAST *type) : Name(name), ArgType(type) {}

};
// FuncTypeAST - Class for function types
class FuncTypeAST : public TypeAST {
	friend class Generator;
	friend class ExternAST;
	friend class CallExprAST;
	friend class FuncExprAST;

	std::vector<FuncArgAST*> Args;
	TypeAST* ReturnType;
	public:
	FuncTypeAST(const std::vector<FuncArgAST*> &args, TypeAST* retType) : Args(args), ReturnType(retType) {};
	virtual llvm::Type *getTy();
	virtual bool eq(TypeAST *other);

	virtual std::string typeDescStr();
};

// RefTypeAST - Class for reference types
class RefTypeAST : public TypeAST {
	friend class Generator;
	friend class ExternAST;
	friend class DerefExprAST;
	friend class ExprAST;
	friend class BinaryExprAST;
	friend class CallExprAST;

	TypeAST* VType;

	RefTypeAST(TypeAST *type) : VType(type) {}

	public:
	static RefTypeAST *Get(TypeAST *type);
	virtual bool eq(TypeAST *other);

	virtual llvm::Type *getTy();

	virtual std::string typeDescStr();
};

#endif

