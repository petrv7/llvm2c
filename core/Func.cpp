#include "Func.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/Metadata.h>
#include <llvm/Support/raw_ostream.h>

#include "../type/Type.h"

#include <utility>
#include <cstdint>
#include <string>
#include <fstream>
#include <set>

const std::set<std::string> STDLIB_FUNCTIONS = {"atof", "atoi", "atol", "strtod", "strtol", "strtoul", "calloc",
                                                "free", "malloc", "realloc", "abort", "atexit", "exit", "getenv",
                                                "system", "bsearch", "qsort", "abs", "div", "labs", "ldiv",
                                                "rand", "srand", "mblen", "mbstowcs", "mbtowc", "wcstombs", "wctomb"};

Func::Func(const llvm::Function* func, Program* program, bool isDeclaration) {
    this->program = program;
    function = func;
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
        if (auto F = llvm::dyn_cast<llvm::Function>(val)) {
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

    std::string name = function->getName().str();
    if (Block::isCFunc(Block::getCFunc(name))) {
        name = Block::getCFunc(name);

        if (Block::isCMath(name)) {
            program->hasMath = true;
            return;
        }
    }

    if (isStdLibFunc(name)) {
        program->hasStdLib = true;
        return;
    }

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

void Func::print() {
    std::string name = function->getName().str();

    if (Block::isCFunc(Block::getCFunc(name))) {
        name = Block::getCFunc(name);
        if (name.compare("va_start") == 0
                || name.compare("va_end") == 0
                || name.compare("va_copy") == 0
                || Block::isCMath(name)) {
            return;
        }
    }

    if (isStdLibFunc(name)) {
        return;
    }

    if (name.substr(0, 4).compare("llvm") == 0) {
        std::replace(name.begin(), name.end(), '.', '_');
    }

    if (name.compare("memcpy") == 0 && function->arg_size() > 3) {
        return;
    }

    returnType->print();
    auto PT = dynamic_cast<PointerType*>(returnType.get());
    if (PT && PT->isArrayPointer) {
        llvm::outs() << " (";
        for (unsigned i = 0; i < PT->levels; i++) {
            llvm::outs() << "*";
        }
        llvm::outs() << name << "(";
    } else {
        llvm::outs() << " " << name << "(";
    }

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

        val->init = true;

    }

    if (isVarArg) {
        if (!first) {
            llvm::outs() << ", ";
        }
        llvm::outs() << "...";
    }

    llvm::outs() << ")";

    if (PT && PT->isArrayPointer) {
        llvm::outs() << ")" + PT->sizes;
    }

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

void Func::saveFile(std::ofstream& file) {
    std::string name = function->getName().str();

    if (Block::isCFunc(Block::getCFunc(name))) {
        name = Block::getCFunc(name);
        if (name.compare("va_start") == 0
                || name.compare("va_end") == 0
                || name.compare("va_copy") == 0
                || Block::isCMath(name)) {
            return;
        }

    }

    if (isStdLibFunc(name)) {
        return;
    }


    if (name.substr(0, 4).compare("llvm") == 0) {
        std::replace(name.begin(), name.end(), '.', '_');
    }

    if (name.compare("memcpy") == 0 && function->arg_size() > 3) {
        return;
    }

    file << returnType->toString();
    auto PT = dynamic_cast<PointerType*>(returnType.get());
    if (PT && PT->isArrayPointer) {
        file << " (";
        for (unsigned i = 0; i < PT->levels; i++) {
            file << "*";
        }
        file << name << "(";
    } else {
        file << " " << name << "(";
    }

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

        val->init = true;
    }

    if (isVarArg) {
        if (!first) {
            file << ", ";
        }
        file << "...";
    }

    file << ")";

    if (PT && PT->isArrayPointer) {
        file << ")" + PT->sizes;
    }

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

void Func::hasMath() {
    program->hasMath = true;
}

bool Func::isStdLibFunc(const std::string& func) {
    return STDLIB_FUNCTIONS.find(func) != STDLIB_FUNCTIONS.end();
}
