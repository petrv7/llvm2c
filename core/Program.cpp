#include "Program.h"

#include <llvm/IR/LLVMContext.h>
#include "llvm/Support/raw_ostream.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Constants.h"

#include "../type/Type.h"

#include <fstream>
#include <exception>
#include <algorithm>

Program::Program(const std::string &file)
    : typeHandler(TypeHandler(this)) {
    fileName = file;
    unsigned idx = fileName.find_last_of("\\/");
    fileName.erase(0, idx + 1);
    idx = fileName.find_last_of("\\.");
    fileName.erase(idx, fileName.size());

    error = llvm::SMDiagnostic();
    module = llvm::parseIRFile(file, error, context);
    if(!module) {
        throw std::invalid_argument("Error loading module - invalid input file \"" + fileName + "\"!\n");
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
                functions.push_back(std::make_unique<Func>(&func, this, false, func.isExternalLinkage(func.getLinkage())));
                declarations.push_back(std::make_unique<Func>(&func, this, true, func.isExternalLinkage(func.getLinkage())));
            }

            if (func.isDeclaration() || llvm::Function::isInternalLinkage(func.getLinkage())) {
                if (func.getName().str().substr(0, 8) != "llvm.dbg") {
                    declarations.push_back(std::make_unique<Func>(&func, this, true, func.isExternalLinkage(func.getLinkage())));
                }
            }
        }
    }
}

void Program::parseGlobalVars() {
    for (const llvm::GlobalVariable& gvar : module->globals()) {
        if (const llvm::Function* F = llvm::dyn_cast<llvm::Function>(&gvar)) {
            continue;
        }

        parseGlobalVar(gvar);
    }
}

void Program::parseGlobalVar(const llvm::GlobalVariable &gvar) {
    bool isPrivate = gvar.hasPrivateLinkage();
    std::string gvarName = gvar.getName().str();
    std::replace(gvarName.begin(), gvarName.end(), '.', '_');

    if (gvarName.compare("index") == 0) {
        gvarName = "g_" + gvarName;
    }

    std::string value;
    if (gvar.hasInitializer()) {
        value = getInitValue(gvar.getInitializer());
    }

    llvm::PointerType* PI = llvm::cast<llvm::PointerType>(gvar.getType());
    globalVars.push_back(std::make_unique<GlobalValue>(gvarName, value, getType(PI->getElementType())));
    globalVars.at(globalVars.size() - 1)->getType()->isStatic = isPrivate;
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
    anonStructCount += 1;
    return name;
}

std::string Program::getInitValue(const llvm::Constant* val) {
    if (llvm::PointerType* PT = llvm::dyn_cast<llvm::PointerType>(val->getType())) {
        std::string name = val->getName().str();
        if (const llvm::GlobalVariable* GV = llvm::dyn_cast<llvm::GlobalVariable>(val)) {
            std::replace(name.begin(), name.end(), '.', '_');

            if (name.compare("index") == 0) {
                name = "g_" + name;
            }
        }

        if (val->getName().str().empty()) {
            return "0";
        }

        if (const llvm::ConstantPointerNull* CPN = llvm::dyn_cast<llvm::ConstantPointerNull>(val)) {
            return "0";
        }

        if (const llvm::FunctionType* FT = llvm::dyn_cast<llvm::FunctionType>(PT->getElementType())) {
            return  "&" + name;
        }

        if (const llvm::StructType* ST = llvm::dyn_cast<llvm::StructType>(PT->getElementType())) {
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
        return std::to_string(CI->getSExtValue());
    }

    if (const llvm::ConstantFP* CFP = llvm::dyn_cast<llvm::ConstantFP>(val)) {
        if (CFP->isInfinity()) {
            this->hasMath = true;
            return "INFINITY";
        }

        if (CFP->isNaN()) {
            hasMath = true;
            return "NAN";
        }

        std::string ret = std::to_string(CFP->getValueAPF().convertToDouble());
        if (ret.compare("-nan") == 0) {
            hasMath = true;
            return "-NAN";
        }

        llvm::SmallVector<char, 32> string;
        ret = "";
        CFP->getValueAPF().toString(string, 32, 0);
        for (int i = 0; i < string.size(); i++) {
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
    unsetAllInit();

    llvm::outs() << getIncludeString();

    if (!structs.empty()) {
        llvm::outs() << "//Struct declarations\n";
        for (const auto& strct : structs) {
            llvm::outs() << "struct " << strct->name << ";\n";
        }
        llvm::outs() << "\n";
    }

    if (typeHandler.hasTypeDefs()) {
        llvm::outs() << "//typedefs\n";
        for (auto elem : typeHandler.sortedTypeDefs) {
            llvm::outs() << elem->defToString() << "\n";
        }
        llvm::outs() << "\n";
    }

    if (!structs.empty()) {
        llvm::outs() << "//Struct definitions\n";
        for (auto& strct : structs) {
            if (!strct->isPrinted) {
                printStruct(strct.get());
            }
        }
        llvm::outs() << "\n";
    }

    if (!unnamedStructs.empty()) {
        llvm::outs() << "//Anonymous struct declarations\n";
        for (const auto& elem : unnamedStructs) {
            llvm::outs() << "struct " << elem.second->name << ";\n";
        }
        llvm::outs() << "\n";
    }

    if (!globalVars.empty()) {
        llvm::outs() << "//Global variable declarations\n";
        for (auto& gvar : globalVars) {
            llvm::outs() << gvar->declToString();
            llvm::outs() << "\n";
        }
        llvm::outs() << "\n";
    }

    if (!declarations.empty()) {
        llvm::outs() << "//Function declarations\n";
        for (const auto& func : declarations) {
            func->print();
        }
        llvm::outs() << "\n";
    }

    if (!unnamedStructs.empty()) {
        llvm::outs() << "//Anonymous struct definitions\n";
        for (auto& elem : unnamedStructs) {
            if (!elem.second->isPrinted) {
                printStruct(elem.second.get());
            }
        }
        llvm::outs() << "\n";
    }

    if (!globalVars.empty()) {
        llvm::outs() << "//Global variable definitions\n";
        for (auto& gvar : globalVars) {
            if (gvar->valueName == "last_index") {
                llvm::outs().flush();
            }
            llvm::outs() << gvar->toString();
            gvar->init = true;
            llvm::outs() << "\n";
        }
        llvm::outs() << "\n";
    }

    if (!functions.empty()) {
        llvm::outs() << "//Function definitions\n";
        for (const auto& func : functions) {
            func->print();
        }
    }
    llvm::outs().flush();
}

void Program::printStruct(Struct* strct) {
    for (auto& item : strct->items) {
        if (auto AT = dynamic_cast<ArrayType*>(item.first.get())) {
            if (AT->isStructArray) {
                printStruct(getStruct(AT->structName));
            }
        }

        if (auto PT = dynamic_cast<PointerType*>(item.first.get())) {
            if (PT->isStructPointer && PT->isArrayPointer) {
                printStruct(getStruct(PT->structName));
            }
        }

        if (auto ST = dynamic_cast<StructType*>(item.first.get())) {
            for (auto& s : structs) {
                if (s->name == ST->name) {
                    printStruct(s.get());
                }
            }

            for (auto& s : unnamedStructs) {
                if (s.second->name == ST->name) {
                    printStruct(s.second.get());
                }
            }
        }
    }
    if (!strct->isPrinted) {
        strct->print();
        strct->isPrinted = true;
        llvm::outs() << "\n\n";
    }
}

void Program::saveStruct(Struct* strct, std::ofstream& file) {
    for (auto& item : strct->items) {
        if (auto AT = dynamic_cast<ArrayType*>(item.first.get())) {
            if (AT->isStructArray) {
                saveStruct(getStruct(AT->structName), file);
            }
        }

        if (auto PT = dynamic_cast<PointerType*>(item.first.get())) {
            if (PT->isStructPointer && PT->isArrayPointer) {
                saveStruct(getStruct(PT->structName), file);
            }
        }

        if (auto ST = dynamic_cast<StructType*>(item.first.get())) {
            for (auto& s : structs) {
                if (s->name == ST->name) {
                    saveStruct(s.get(), file);
                }
            }

            for (auto& s : unnamedStructs) {
                if (s.second->name == ST->name) {
                    saveStruct(s.second.get(), file);
                }
            }
        }
    }
    if (!strct->isPrinted) {
        file << strct->toString();
        strct->isPrinted = true;
        file << "\n\n";
    }
}

void Program::saveFile(const std::string& fileName) {
    unsetAllInit();

    std::ofstream file;
    file.open(fileName);

    file << getIncludeString();

    if (!structs.empty()) {
        file << "//Struct declarations\n";
        for (auto& strct : structs) {
            file << "struct " << strct->name << ";\n";
        }
        file << "\n";
    }

    if (typeHandler.hasTypeDefs()) {
        file << "//typedefs\n";
        for (auto elem : typeHandler.sortedTypeDefs) {
            file << elem->defToString() << "\n";
        }
        file << "\n";
    }

    if (!structs.empty()) {
        file << "//Struct definitions\n";
        for (auto& strct : structs) {
            if (!strct->isPrinted) {
                saveStruct(strct.get(), file);
            }
        }
        file << "\n";
    }

    if (!unnamedStructs.empty()) {
        file << "//Anonymous struct declarations\n";
        for (const auto& elem : unnamedStructs) {
            file << "struct " << elem.second->name << ";\n";
        }
        file << "\n";
    }

    if (!globalVars.empty()) {
        file << "//Global variable declarations\n";
        for (auto& gvar : globalVars) {
            file << gvar->declToString();
            file << "\n";
        }
        file << "\n";
    }

    if (!declarations.empty()) {
        file << "//Function declarations\n";
        for (const auto& func : declarations) {
            func->saveFile(file);
        }
        file << "\n";
    }

    if (!unnamedStructs.empty()) {
        file << "//Anonymous struct definitions\n";
        for (auto& elem : unnamedStructs) {
            if (!elem.second->isPrinted) {
                saveStruct(elem.second.get(), file);
            }
        }
        file << "\n";
    }

    if (!globalVars.empty()) {
        file << "//Global variable definitions\n";
        for (auto& gvar : globalVars) {
            file << gvar->toString();
            gvar->init = true;
            file << "\n";
        }
        file << "\n";
    }

    file << "//Function definitions\n";
    for (const auto& func : functions) {
        func->saveFile(file);
    }

    file.close();

    llvm::outs() << "Translated program successfuly saved into " << fileName << "\n";
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
    declarations.push_back(std::make_unique<Func>(func, this, true, func->isExternalLinkage(func->getLinkage())));
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

std::unique_ptr<Type> Program::getType(const llvm::Type* type, bool voidType) {
    return typeHandler.getType(type, voidType);
}

std::string Program::getIncludeString() const {
    std::string ret;

    if (hasMath) {
        ret+= "#include \"math.h\"\n";
    }

    if (hasVarArg) {
        ret += "#include <stdarg.h>\n";
    }

    if (!ret.empty()) {
        ret += "\n";
    }

    return ret;
}
