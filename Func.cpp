#include "Func.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/Metadata.h>
#include <llvm/Support/raw_ostream.h>

#include <utility>
#include <cstdint>
#include <string>
#include <fstream>

Func::Func(llvm::Function* func) {
    function = func;
    varCount = 0;
    blockCount = 0;

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
    auto iter = exprMap.find(val);

    if (iter != exprMap.end()) {
        return exprMap.find(val)->second.get();
    }

    return nullptr;
}

void Func::createExpr(const llvm::Value* val, std::unique_ptr<Expr> expr) {
    exprMap[val] = std::move(expr);
}

void Func::createExpr(const llvm::Instruction* ins, std::unique_ptr<Expr> expr) {
    createExpr(static_cast<const llvm::Value*>(ins), std::move(expr));
}

std::string Func::getVarName() {
    std::string varName = "var";
    varName += std::to_string(varCount);
    varCount++;

    return varName;
}

void Func::parseFunction() {
    for (const llvm::Value& arg : function->args()) {
        std::string varName = "var";
        varName += std::to_string(varCount);
        exprMap[&arg] = std::make_unique<Value>(varName, std::move(getType(arg.getType())));
        varCount++;
    }

    for (const auto& block : *function) {
        getBlockName(&block);
        blockMap[&block]->parseLLVMBlock();
    }
}

void Func::print() const {
    llvm::outs() << function->getName().str() << "(";
    bool first = true;

    for (const llvm::Value& arg : function->args()) {
        if (first) {
            exprMap.find(&arg)->second->print();
        } else {
            llvm::outs() << ", ";
            exprMap.find(&arg)->second->print();
        }
        first = false;
    }

    llvm::outs() << ") {\n";

    first = true;
    for (const auto& block : *function) {
        if (!first) {
            llvm::outs() << blockMap.find(&block)->second->blockName;
            llvm::outs() << ":\n";
        }
        blockMap.find(&block)->second->print();
        first = false;
    }

    llvm::outs() << "}\n\n";
}

void Func::saveFile(std::ofstream& file) const {
    file << function->getName().str() << "(";
    bool first = true;

    for (const llvm::Value& arg : function->args()) {
        if (first) {
            file << exprMap.find(&arg)->second->toString();
        } else {
            file << ", ";
            file << exprMap.find(&arg)->second->toString();
        }
        first = false;
    }

    file << ") {\n";

    first = true;
    for (const auto& block : *function) {
        if (!first) {
            file << blockMap.find(&block)->second->blockName;
            file << ":\n";
        }
        blockMap.find(&block)->second->saveFile(file);
        first = false;
    }

    file << "}\n\n";
}

std::unique_ptr<Type> Func::getType(const llvm::Type* type, bool isArray, unsigned int size) {
    if (isArray) {
        if (type->getArrayElementType()->isArrayTy()) {
            return std::make_unique<ArrayType>(std::move(getType(type->getArrayElementType(), true, type->getArrayElementType()->getArrayNumElements())), size);
        } else {
            return std::make_unique<ArrayType>(std::move(getType(type->getArrayElementType())), size);
        }
    }

    if (type->isVoidTy()) {
        return std::make_unique<VoidType>();
    }

    if (type->isIntegerTy()) {
        const auto intType = static_cast<const llvm::IntegerType*>(type);
        switch(intType->getBitWidth()) {
        case 8:
            return std::make_unique<CharType>(false);
        case 16:
            return std::make_unique<ShortType>(false);
        case 32:
            return std::make_unique<IntType>(false);
        case 64:
            return std::make_unique<LongType>(false);
        default:
            return nullptr;
        }
    }

    if (type->isFloatTy()) {
        return std::make_unique<FloatType>();
    }

    if (type->isDoubleTy()) {
        return std::make_unique<DoubleType>();
    }

    if (type->isX86_FP80Ty()) {
        return std::make_unique<LongDoubleType>();
    }

    if (type->isPointerTy()) {
        const auto ptr = static_cast<const llvm::PointerType*>(type);
        return std::make_unique<PointerType>(std::move(getType(ptr->getElementType())));
    }

    if (type->isStructTy()) {
        const llvm::StructType* structType = llvm::dyn_cast<const llvm::StructType>(type);
        return std::make_unique<StructType>(structType->getName());
    }

    return nullptr;
}
