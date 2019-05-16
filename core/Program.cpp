#include "Program.h"

#include <llvm/IR/LLVMContext.h>
#include "llvm/Support/raw_ostream.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Constants.h"

#include "../type/Type.h"

#include <fstream>
#include <exception>
#include <algorithm>
#include <regex>
#include <iostream>

Program::Program(const std::string &file, bool includes, bool casts)
    : typeHandler(TypeHandler(this)),
      includes(includes),
      noFuncCasts(casts) {
    error = llvm::SMDiagnostic();
    module = llvm::parseIRFile(file, error, context);
    if(!module) {
        throw std::invalid_argument("Error loading module - invalid input file:\n" + file + "\n");
    }

    llvm::outs() << "IR file successfuly parsed.\n";

    parseProgram();
}

void Program::parseProgram() {
    llvm::outs() << "Translating module...\n";

    parseGlobalVars();
    parseStructs();
    parseFunctions();

    llvm::outs() << "Module successfuly translated.\n";

    if (stackIgnored) {
        llvm::outs() << "Intrinsic stacksave/stackrestore ignored!\n";
    }

    llvm::outs() << "\n";
}

void Program::parseStructs() {
    for (llvm::StructType* structType : module->getIdentifiedStructTypes()) {
        std::string structName = TypeHandler::getStructName(structType->getName().str());

        if (structName.compare("__va_list_tag") == 0) {
            hasVarArg = true;
            auto structExpr = std::make_unique<Struct>(structName);
            structExpr->addItem(std::make_unique<IntType>(true), "gp_offset");
            structExpr->addItem(std::make_unique<IntType>(true), "fp_offset");
            structExpr->addItem(std::make_unique<PointerType>(std::make_unique<VoidType>()), "overflow_arg_area");
            structExpr->addItem(std::make_unique<PointerType>(std::make_unique<VoidType>()), "reg_save_area");
            structs.push_back(std::move(structExpr));
            continue;
        }

        auto structExpr = std::make_unique<Struct>(structName);

        for (llvm::Type* type : structType->elements()) {
            structExpr->addItem(getType(type), getStructVarName());
        }

        structs.push_back(std::move(structExpr));
    }
}

void Program::parseFunctions() {
    for(const llvm::Function& func : module->functions()) {
        if (func.hasName()) {
            if (!func.isDeclaration()) {
                functions[&func] = std::make_unique<Func>(&func, this, false);
                if (!declarations.count(&func)) {
                    declarations[&func] = std::make_unique<Func>(&func, this, true);
                }
            }

            if (func.isDeclaration() || llvm::Function::isInternalLinkage(func.getLinkage())) {
                if (func.getName().str().substr(0, 8) != "llvm.dbg") {
                    if (!declarations.count(&func)) {
                        declarations[&func] = std::make_unique<Func>(&func, this, true);
                    }
                }
            }
        }
    }
}

void Program::parseGlobalVars() {
    for (const llvm::GlobalVariable& gvar : module->globals()) {
        if (llvm::isa<llvm::Function>(&gvar)) {
            continue;
        }

        parseGlobalVar(gvar);
    }
}

void Program::parseGlobalVar(const llvm::GlobalVariable &gvar) {
    std::string gvarName = gvar.getName().str();
    std::replace(gvarName.begin(), gvarName.end(), '.', '_');

    std::regex varName("var[0-9]+");
    if (std::regex_match(gvarName, varName)) {
        globalVarNames.insert(gvarName);
    }

    std::string value;
    if (gvar.hasInitializer()) {
        value = getInitValue(gvar.getInitializer());
    }

    llvm::PointerType* PT = llvm::cast<llvm::PointerType>(gvar.getType());
    globalVars.push_back(std::make_unique<GlobalValue>(gvarName, value, getType(PT->getElementType())));
    globalVars.at(globalVars.size() - 1)->getType()->isStatic = gvar.hasInternalLinkage();
    globalRefs[&gvar] = std::make_unique<RefExpr>(globalVars.at(globalVars.size() - 1).get());
}

std::string Program::getStructVarName() {
    std::string varName = "structVar";
    varName += std::to_string(structVarCount);
    structVarCount++;

    return varName;
}

std::string Program::getAnonStructName() {
    std::string name = "anonymous_struct" + std::to_string(anonStructCount);
    anonStructCount++;
    return name;
}

std::string Program::getInitValue(const llvm::Constant* val) {
    if (llvm::PointerType* PT = llvm::dyn_cast<llvm::PointerType>(val->getType())) {
        std::string name = val->getName().str();
        if (llvm::isa<llvm::GlobalVariable>(val)) {
            std::replace(name.begin(), name.end(), '.', '_');
        }

        if (val->getName().str().empty()) {
            return "0";
        }

        if (llvm::isa<llvm::ConstantPointerNull>(val)) {
            return "0";
        }

        if (llvm::isa<llvm::FunctionType>(PT->getElementType())) {
            return  "&" + name;
        }

        if (llvm::isa<llvm::StructType>(PT->getElementType())) {
            return "&" + name;
        }

        if (const llvm::GlobalVariable* GV = llvm::dyn_cast<llvm::GlobalVariable>(val->getOperand(0))) {
            auto RE = static_cast<RefExpr*>(globalRefs[GV].get());
            auto GVAL = static_cast<GlobalValue*>(RE->expr);
            if (!GVAL->isDefined) {
                parseGlobalVar(*GV);
            }

            GVAL->isDefined = true;
            return GVAL->toString();
        }

        return "&" + name;
    }

    if (const llvm::ConstantInt* CI = llvm::dyn_cast<llvm::ConstantInt>(val)) {
        std::string value;
        if (CI->getBitWidth() > 64) {
            const llvm::APInt& API = CI->getValue();
            value = std::to_string(API.getLimitedValue());
        } else if (CI->getBitWidth() == 1) { //bool in LLVM
            value = std::to_string(-1 * CI->getSExtValue());
        } else {
            value = std::to_string(CI->getSExtValue());
        }

        return value;
    }

    if (const llvm::ConstantFP* CFP = llvm::dyn_cast<llvm::ConstantFP>(val)) {
        if (CFP->isInfinity()) {
            return "__builtin_inff ()";
        }

        if (CFP->isNaN()) {
            return "__builtin_nanf (\"\")";
        }

        std::string ret = std::to_string(CFP->getValueAPF().convertToDouble());
        if (ret.compare("-nan") == 0) {
            return "-(__builtin_nanf (\"\"))";
        }

        llvm::SmallVector<char, 32> string;
        ret = "";
        CFP->getValueAPF().toString(string, 32, 0);
        for (unsigned i = 0; i < string.size(); i++) {
            ret += string[i];
        }

        return ret;
    }

    if (const llvm::ConstantDataArray* CDA = llvm::dyn_cast<llvm::ConstantDataArray>(val)) {
        std::string value = "{";
        bool first = true;

        for (unsigned i = 0; i < CDA->getNumElements(); i++) {
            if (!first) {
                value += ", ";
            }
            first = false;

            value += getInitValue(CDA->getElementAsConstant(i));
        }

        return value + "}";
    }

    if (const llvm::ConstantStruct* CS = llvm::dyn_cast<llvm::ConstantStruct>(val)) {
        std::string value = "{";
        bool first = true;
        for (unsigned i = 0; i < CS->getNumOperands(); i++) {
            if (!first) {
                value += ", ";
            }
            first = false;

            value += getInitValue(llvm::cast<llvm::Constant>(val->getOperand(i)));
        }

        return value + "}";
    }

    if (!val->getType()->isStructTy() && !val->getType()->isPointerTy() && !val->getType()->isArrayTy()) {
        return "0";
    }

    return "{}";
}

void Program::unsetAllInit() {
    for (auto& gvar : globalVars) {
        gvar->init = false;
    }

    for (auto& strct : structs) {
        strct->isPrinted = false;
    }
}

void Program::print() {
    output(std::cout);
}

void Program::saveFile(const std::string& fileName) {
    std::ofstream file;
    file.open(fileName);

    if (!file.is_open()) {
        throw std::invalid_argument("Output file cannot be opened!");
    }

    output(file);

    file.close();

    std::cout << "Translated program successfuly saved into " << fileName << "\n";
}

void Program::outputStruct(Struct* strct, std::ostream& stream) {
    for (auto& item : strct->items) {
        if (auto AT = dynamic_cast<ArrayType*>(item.first.get())) {
            if (AT->isStructArray) {
                outputStruct(getStruct(AT->structName), stream);
            }
        }

        if (auto PT = dynamic_cast<PointerType*>(item.first.get())) {
            if (PT->isStructPointer && PT->isArrayPointer) {
                outputStruct(getStruct(PT->structName), stream);
            }
        }

        if (auto ST = dynamic_cast<StructType*>(item.first.get())) {
            for (auto& s : structs) {
                if (s->name == ST->name) {
                    outputStruct(s.get(), stream);
                }
            }

            for (auto& s : unnamedStructs) {
                if (s.second->name == ST->name) {
                    outputStruct(s.second.get(), stream);
                }
            }
        }
    }

    if (!strct->isPrinted) {
        stream << strct->toString();
        strct->isPrinted = true;
        stream << "\n\n";
    }
}

void Program::output(std::ostream &stream) {
    unsetAllInit();

    stream << getIncludeString();

    if (!structs.empty()) {
        stream << "//Struct declarations\n";
        for (auto& strct : structs) {
            stream << "struct " << strct->name << ";\n";
        }
        stream << "\n";
    }

    if (typeHandler.hasTypeDefs()) {
        stream << "//typedefs\n";
        for (auto elem : typeHandler.sortedTypeDefs) {
            stream << elem->defToString() << "\n";
        }
        stream << "\n";
    }

    if (!structs.empty()) {
        stream << "//Struct definitions\n";
        for (auto& strct : structs) {
            if (!strct->isPrinted) {
                outputStruct(strct.get(), stream);
            }
        }
        stream << "\n";
    }

    if (!unnamedStructs.empty()) {
        stream << "//Anonymous struct declarations\n";
        for (const auto& elem : unnamedStructs) {
            stream << "struct " << elem.second->name << ";\n";
        }
        stream << "\n";
    }

    if (!globalVars.empty()) {
        stream << "//Global variable declarations\n";
        for (auto& gvar : globalVars) {
            if (includes && (gvar->valueName.compare("stdin") == 0 || gvar->valueName.compare("stdout") == 0 || gvar->valueName.compare("stderr") == 0)) {
                continue;
            }

            stream << gvar->declToString();
            stream << "\n";
        }
        stream << "\n";
    }

    if (!declarations.empty()) {
        stream << "//Function declarations\n";
        for (const auto& func : declarations) {
            func.second->output(stream);
        }
        stream << "\n";
    }

    if (!unnamedStructs.empty()) {
        stream << "//Anonymous struct definitions\n";
        for (auto& elem : unnamedStructs) {
            if (!elem.second->isPrinted) {
                outputStruct(elem.second.get(), stream);
            }
        }
        stream << "\n";
    }

    if (!globalVars.empty()) {
        stream << "//Global variable definitions\n";
        for (auto& gvar : globalVars) {
            if (includes && (gvar->valueName.compare("stdin") == 0 || gvar->valueName.compare("stdout") == 0 || gvar->valueName.compare("stderr") == 0)) {
                gvar->init = true;
                continue;
            }

            stream << gvar->toString();
            gvar->init = true;
            stream << "\n";
        }
        stream << "\n";
    }

    stream << "//Function definitions\n";
    for (const auto& func : functions) {
        func.second->output(stream);
    }
}

Struct* Program::getStruct(const llvm::StructType* strct) const {
    std::string structName = TypeHandler::getStructName(strct->getName().str());

    for (const auto& structElem : structs) {
        if (structElem->name.compare(structName) == 0) {
            return structElem.get();
        }
    }

    if (unnamedStructs.find(strct) != unnamedStructs.end()) {
        return unnamedStructs.find(strct)->second.get();
    }

    return nullptr;
}

Struct* Program::getStruct(const std::string& name) const {
    for (const auto& structElem : structs) {
        if (structElem->name.compare(name) == 0) {
            return structElem.get();
        }
    }

    for (const auto& mapElem : unnamedStructs) {
        if (mapElem.second->name.compare(name) == 0) {
            return mapElem.second.get();
        }
    }

    return nullptr;
}

RefExpr* Program::getGlobalVar(const llvm::Value* val) {
    if (const llvm::GlobalVariable* GV = llvm::dyn_cast<llvm::GlobalVariable>(val)) {
        if (globalRefs.count(GV)) {
            return globalRefs.find(GV)->second.get();
        }
    }

    return nullptr;
}

void Program::addDeclaration(llvm::Function* func) {
    if (!declarations.count(func)) {
        declarations[func] = std::make_unique<Func>(func, this, true);
    }
}

void Program::createNewUnnamedStruct(const llvm::StructType *strct) {
    if (unnamedStructs.find(strct) != unnamedStructs.end()) {
        return;
    }

    auto structExpr = std::make_unique<Struct>(getAnonStructName());

    for (llvm::Type* type : strct->elements()) {
        structExpr->addItem(getType(type), getStructVarName());
    }

    unnamedStructs[strct] = std::move(structExpr);
}

std::unique_ptr<Type> Program::getType(const llvm::Type* type) {
    return typeHandler.getType(type);
}

std::string Program::getIncludeString() const {
    std::string ret;

    if (hasVarArg) {
        ret += "#include <stdarg.h>\n";
    }

    if (hasStdLib) {
        ret += "#include <stdlib.h>\n";
    }

    if (hasString) {
        ret += "#include <string.h>\n";
    }

    if (hasStdio) {
        ret += "#include <stdio.h>\n";
    }

    if (hasPthread) {
        ret += "#include <pthread.h>\n";
    }

    if (!ret.empty()) {
        ret += "\n";
    }

    return ret;
}
