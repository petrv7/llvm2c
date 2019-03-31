#include "Func.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/Metadata.h>
#include <llvm/Support/raw_ostream.h>

#include "Type/Type.h"

#include <utility>
#include <cstdint>
#include <string>
#include <fstream>

Func::Func(llvm::Function* func, Program* program, bool isDeclaration) {
    this->program = program;
    function = func;
    varCount = 0;
    blockCount = 0;
    this->isDeclaration = isDeclaration;
    returnType = getType(func->getReturnType());

    parseFunction();
}

std::string Func::getBlockName(const llvm::BasicBlock* block) {
    auto iter = blockMap.find(block);
    if (iter == blockMap.end()) {
        std::string blockName = "block";
        blockName += std::to_string(blockCount);
        blockMap[block] = std::make_unique<Block>(blockName, block, this);
        blockCount++;

        return blockName;
    }

    return iter->second->blockName;
}

Expr* Func::getExpr(const llvm::Value* val) {
    if (exprMap.find(val) == exprMap.end()) {
        if (llvm::Function* F = llvm::dyn_cast<llvm::Function>(val)) {
            createExpr(val, std::make_unique<Value>("&" + F->getName().str(), getType(F->getReturnType())));
            return exprMap.find(val)->second.get();
        }
    } else {
        return exprMap.find(val)->second.get();
    }

    return program->getGlobalVar(val);
}

void Func::createExpr(const llvm::Value* val, std::unique_ptr<Expr> expr) {
    exprMap[val] = std::move(expr);
}

std::string Func::getVarName() {
    std::string varName = "var";
    varName += std::to_string(varCount);
    varCount++;

    return varName;
}

void Func::parseFunction() {
    const llvm::Value* larg;
    isVarArg = false;

    for (const llvm::Value& arg : function->args()) {
        std::string varName = "var";
        varName += std::to_string(varCount);
        exprMap[&arg] = std::make_unique<Value>(varName, getType(arg.getType()));
        varCount++;
        larg = &arg;
    }

    lastArg = exprMap[larg].get();
    if (lastArg) {
        isVarArg = function->isVarArg();
    }

    for (const auto& block : *function) {
        getBlockName(&block);
    }

    for (const auto& block : *function) {
        blockMap[&block]->parseLLVMBlock();
    }
}

void Func::print() const {
    std::string name = function->getName().str();
    if (Block::isCFunc(Block::getCFunc(name))) {
        name = Block::getCFunc(name);
        if (name.compare("va_start") == 0 || name.compare("va_end") == 0) {
            return;
        }
    }
    if (name.substr(0, 4).compare("llvm") == 0) {
        std::replace(name.begin(), name.end(), '.', '_');
    }

    returnType->print();
    llvm::outs() <<  " " << name << "(";

    bool first = true;
    for (const llvm::Value& arg : function->args()) {
        if (!first) {
            llvm::outs() << ", ";
        }
        first = false;

        Value* val = static_cast<Value*>(exprMap.find(&arg)->second.get());
        val->getType()->print();
        llvm::outs() << " ";
        val->print();
    }

    if (isVarArg) {
        if (!first) {
            llvm::outs() << ", ";
        }
        llvm::outs() << "...";
    }


    llvm::outs() << ")";

    if (isDeclaration) {
        llvm::outs() << ";\n";
        return;
    }

    llvm::outs() << " {\n";

    first = true;
    for (const auto& block : *function) {
        if (!first) {
            llvm::outs() << blockMap.find(&block)->second->blockName;
            llvm::outs() << ":\n    ;\n";
        }
        blockMap.find(&block)->second->print();
        first = false;
    }

    llvm::outs() << "}\n\n";
}

void Func::saveFile(std::ofstream& file) const {
    std::string name = function->getName().str();
    if (Block::isCFunc(Block::getCFunc(name))) {
        name = Block::getCFunc(name);
        if (name.compare("va_start") == 0 || name.compare("va_end") == 0) {
            return;
        }
    }
    if (name.substr(0, 4).compare("llvm") == 0) {
        std::replace(name.begin(), name.end(), '.', '_');
    }

    file << returnType->toString();
    file <<  " " << name << "(";
    bool first = true;

    for (const llvm::Value& arg : function->args()) {
        if (!first) {
            file << ", ";
        }
        first = false;

        Value* val = static_cast<Value*>(exprMap.find(&arg)->second.get());
        file << val->getType()->toString();
        file << " ";
        file << val->toString();
    }

    if (isVarArg) {
        if (!first) {
            file << ", ";
        }
        file << "...";
    }

    file << ")";

    if (isDeclaration) {
        file << ";\n";
        return;
    }

    file << " {\n";

    first = true;
    for (const auto& block : *function) {
        if (!first) {
            file << blockMap.find(&block)->second->blockName;
            file << ":\n    ;\n";
        }
        blockMap.find(&block)->second->saveFile(file);
        first = false;
    }

    file << "}\n\n";
}

Struct* Func::getStruct(const llvm::StructType* strct) const {
    return program->getStruct(strct);
}

Struct* Func::getStruct(const std::string& name) const {
    return program->getStruct(name);
}

RefExpr* Func::getGlobalVar(llvm::Value* val) const {
    return program->getGlobalVar(val);
}

void Func::addDeclaration(llvm::Function* func) {
    program->addDeclaration(func);
}

void Func::stackIgnored() {
    program->stackIgnored = true;
}

void Func::createNewUnnamedStruct(const llvm::StructType* strct) {
    program->createNewUnnamedStruct(strct);
}

std::unique_ptr<Type> Func::getType(const llvm::Type* type, bool voidType) {
    return program->getType(type, voidType);
}
