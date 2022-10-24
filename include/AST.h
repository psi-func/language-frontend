#pragma once

#include "parser.h"

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"

using namespace llvm;

/// ExprAST - Base class for all expression nodes.
class ExprAST
{
public:
    virtual ~ExprAST() = default;
    virtual Value *codegen() = 0;
};

/// NumberExprAST - Expression class for numeric litarals like "1.0"
class NumberExprAST : public ExprAST
{
    double Val;

public:
    NumberExprAST(double Val) : Val(Val) {}
    Value* codegen() override {
        return ConstantFP::get(*TheContext, APFloat(Val));
    }
};

/// VariableExprAST - Expression class fr a binary operator.
class VariableExprAST : public ExprAST
{
    std::string Name;

public:
    VariableExprAST(const std::string &Name) : Name(Name) {}

    Value* codegen() override {
        Value *V = NamedValues[Name];
        if (!V)
            LogErrorV("Unknown variable name");
        return V;
    }
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST
{
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(char op,
                  std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS)
        : Op(op),
          LHS(std::move(LHS)),
          RHS(std::move(RHS)) {}

    Value* codegen() override {
        Value *L = LHS->codegen();
        Value *R = RHS->codegen();
        if (!(L && R))
            return nullptr;
        
        switch(Op) {
            case '+':
                return Builder->CreateFAdd(L, R, "addtmp");
            case '-':
                return Builder->CreateFSub(L, R, "subtmp");
            case '*':
                return Builder->CreateFMul(L, R, "multmp");
            case '<':
                L = Builder->CreateFCmpULT(L, R, "cmptmp");
                return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
            default:
                return LogErrorV("invalid binaru operator");
        }
    }
};

/// CallExprAST - Expression class fr function calls.
class CallExprAST : public ExprAST
{
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

public:
    CallExprAST(const std::string &Callee, std::vector<std::unique_ptr<ExprAST>> Args) : Callee(Callee), Args(std::move(Args)) {}

    Value* codegen() override {
        Function *CalleeF = TheModule->getFunction(Callee);
        if (!CalleeF)
            return LogErrorV("Unknown function referenced");

        if (CalleeF->arg_size() != Args.size()) 
            return LogErrorV("Incorrect # arguments passed");
        
        std::vector<Value*> ArgsV;
        for (int i = 0, e = Args.size(); i != e; ++i) {
            ArgsV.push_back(Args[i]->codegen());
            if (!ArgsV.back()) 
                return nullptr;
        }
        return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
    }
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST
{
    std::string Name;
    std::vector<std::string> Args;

public:
    PrototypeAST(const std::string &name, std::vector<std::string> Args) : Name(name), Args(std::move(Args)) {}

    Function* codegen() {
        std::vector<Type*> Doubles(Args.size(), Type::getDoubleTy(*TheContext));

        FunctionType* FT = 
            FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);
        
        Function *F = 
            Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());
        
        unsigned int Idx = 0;
        for(auto &Arg: F->args()) {
            Arg.setName(Args[Idx++]);
        }

        return F;

    }


    const std::string &getName() const { return Name; }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST
{
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

public:
    FunctionAST(
        std::unique_ptr<PrototypeAST> Proto,
        std::unique_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}

    Function* codegen() {
        Function* TheFunction = TheModule->getFunction(Proto->getName());
        
        if (!TheFunction) 
            TheFunction = Proto->codegen();
        
        if (!TheFunction)
            return nullptr;

        if (!TheFunction->empty())
            return (Function*)LogErrorV("Function cannot be redefined"); 

        BasicBlock* BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
        Builder->SetInsertPoint(BB);

        NamedValues.clear();
        for (auto &Arg: TheFunction->args())
            NamedValues[std::string(Arg.getName())] = &Arg;

        if (Value* RetVal = Body->codegen()) {
            Builder->CreateRet(RetVal);

            verifyFunction(*TheFunction);

            return TheFunction;
        }

        TheFunction->eraseFromParent();
        return nullptr;
    }
};
