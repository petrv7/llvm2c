#include "Program.h"

#include <llvm/IR/LLVMContext.h>
#include "llvm/Support/raw_ostream.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Constants.h"

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
    gvarCount = 0;
}

void Program::parseProgram() {
    parseGlobalVars();
    parseStructs();
    parseFunctions();
}

void Program::parseStructs() {
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
}

void Program::parseFunctions() {
    for(llvm::Function& func : module->functions()) {
        if (func.hasName()) {
            if (func.getName().str().compare("llvm.dbg.declare") == 0) {
                continue;
            }
        }
        functions.push_back(std::move(std::make_unique<Func>(&func, this)));
    }
}

void Program::parseGlobalVars() {
    for (const llvm::GlobalVariable& gvar : module->globals()) {
        std::string gvarName;
        if (gvar.hasName()) {
            gvarName = "&" + gvar.getName().str();
        } else {
            gvarName = getGvarName();
        }

        std::string value = "";
        if (gvar.hasInitializer()) {
            value = getValue(gvar.getInitializer());
        }

        llvm::PointerType* PI = llvm::cast<llvm::PointerType>(gvar.getType());
        if (PI->getElementType()->isArrayTy()) {
            unsigned int size = PI->getElementType()->getArrayNumElements();
            globalVars[&gvar] = std::make_unique<GlobalValue>(gvarName, value, std::move(Type::getType(PI->getElementType(), true, size)));
        } else {
            globalVars[&gvar] = std::make_unique<GlobalValue>(gvarName, value, std::move(Type::getType(PI->getElementType())));
        }
    }
}

std::string Program::getStructVarName() {
    std::string varName = "structVar";
    varName += std::to_string(structVarCount);
    structVarCount++;

    return varName;
}

std::string Program::getGvarName() {
    std::string varName = "&gvar";
    varName += std::to_string(gvarCount);
    structVarCount++;

    return varName;
}

std::string Program::getValue(const llvm::Constant* val) const {
    if (llvm::ConstantInt* CI = llvm::dyn_cast<llvm::ConstantInt>(val)) {
        return std::to_string(CI->getSExtValue());
    }
    if (llvm::ConstantFP* CFP = llvm::dyn_cast<llvm::ConstantFP>(val)) {
        return std::to_string(CFP->getValueAPF().convertToFloat());
    }
    if (llvm::ConstantDataArray* CDA = llvm::dyn_cast<llvm::ConstantDataArray>(val)) {
        std::string value = "{";
        bool first = true;

        for (unsigned i = 0; i < CDA->getNumElements(); i++) {
            if (!first) {
                value += ", ";
            }
            first = false;

            value += getValue(CDA->getElementAsConstant(i));
        }

        return value + "}";
    }
}

void Program::print() const {
    for (auto it = structs.rbegin(); it != structs.rend(); it++) {
        it->get()->print();
        llvm::outs() << "\n";
    }
    llvm::outs() << "\n";

    for (auto& global : module->globals()) {
        globalVars[&global]->print();
        globalVars[&global]->init = true;
        llvm::outs() << "\n";
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
        file << structExpr->toString() << "\n";
    }
    file << "\n";

    for (auto& global : module->globals()) {
        file << globalVars[&global]->toString() << "\n";
        globalVars[&global]->init = true;
    }
    file << "\n";

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

GlobalValue* Program::getGlobalVar(llvm::Value* val) const {
    llvm::GlobalVariable* GV = llvm::cast<llvm::GlobalVariable>(val);
    return globalVars[GV].get();
}
