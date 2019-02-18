#include "Program.h"

#include <llvm/IR/LLVMContext.h>
#include "llvm/Support/raw_ostream.h"
#include "llvm/IRReader/IRReader.h"

#include "Type.h"

#include <fstream>
#include <exception>

Program::Program(const std::string &file) {
    error = llvm::SMDiagnostic();
    module = llvm::parseIRFile(file, error, context);
    if(!module) {
        throw std::invalid_argument("Error loading module!");
    }
    structVarCount = 0;
}

void Program::parseProgram() {
    for (llvm::StructType* structType : module->getIdentifiedStructTypes()) {
        std::string name = "";
        if (structType->hasName()) {
            name = structType->getName().str();
            name.erase(0, 7);
        }

        auto structExpr = std::make_unique<Struct>(name);

        for (llvm::Type* type : structType->elements()) {
            if (type->isArrayTy()) {
                unsigned int size = type->getArrayNumElements();
                structExpr->addItem(std::move(Type::getType(type, true, size)), getStructVarName());
            } else {
                structExpr->addItem(std::move(Type::getType(type)), getStructVarName());
            }
        }

        structs.push_back(std::move(structExpr));
    }

    for(llvm::Function& func : module->functions()) {
        if (func.hasName()) {
            if (func.getName().str().compare("llvm.dbg.declare") == 0) {
                continue;
            }
        }
        functions.push_back(std::move(std::make_unique<Func>(&func, this)));
    }
}

std::string Program::getStructVarName() {
    std::string varName = "structVar";
    varName += std::to_string(structVarCount);
    structVarCount++;

    return varName;
}

void Program::print() const {
    for (const auto& structExpr : structs) {
        structExpr->print();
    }

    llvm::outs() << "\n";

    for (const auto& func : functions) {
        func->print();
    }
}

void Program::saveFile(const std::string& fileName) const {
    std::ofstream file;
    file.open(fileName);

    for (const auto& structExpr : structs) {
        file << structExpr->toString();
    }

    for (const auto& func : functions) {
        func->saveFile(file);
    }

    file.close();
}

Struct* Program::getStruct(const std::string& name) const {
    for (const auto& strct : structs) {
        if (strct->name == name) {
            return strct.get();
        }
    }

    return nullptr;
}
