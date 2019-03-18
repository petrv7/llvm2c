#include "Program.h"

#include <llvm/IR/LLVMContext.h>
#include "llvm/Support/raw_ostream.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Constants.h"

#include "Type/Type.h"

#include <fstream>
#include <exception>
#include <algorithm>

Program::Program(const std::string &file) {
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
    structVarCount = 0;
    gvarCount = 0;
    hasVarArg = false;
    stackIgnored = false;

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
        std::string name = "";
        if (structType->hasName()) {
            name = structType->getName().str();
            if (name.substr(0, 6).compare("struct") == 0) {
                name.erase(0, 7);
            } else {
                //union
                name.erase(0, 6);
            }
        }

        if (name.compare("__va_list_tag") == 0) {
            hasVarArg = true;
            auto structExpr = std::make_unique<Struct>(name);
            structExpr->addItem(std::make_unique<IntType>(true), "gp_offset");
            structExpr->addItem(std::make_unique<IntType>(true), "fp_offset");
            structExpr->addItem(std::make_unique<PointerType>(std::make_unique<VoidType>()), "overflow_arg_area");
            structExpr->addItem(std::make_unique<PointerType>(std::make_unique<VoidType>()), "reg_save_area");
            structs.push_back(std::move(structExpr));
            continue;
        }

        auto structExpr = std::make_unique<Struct>(name);

        for (llvm::Type* type : structType->elements()) {
            structExpr->addItem(Type::getType(type), getStructVarName());
        }

        structs.push_back(std::move(structExpr));
    }
}

void Program::parseFunctions() {
    for(const llvm::Function& func : module->functions()) {
        if (func.hasName()) {
            if (func.isDeclaration()) {
                if (func.getName().str().substr(0, 8) != "llvm.dbg") {
                    declarations.push_back(std::make_unique<Func>(&func, this, true));
                }
            } else {
                functions.push_back(std::make_unique<Func>(&func, this, false));
            }
        }
    }
}

void Program::parseGlobalVars() {
    for (const llvm::GlobalVariable& gvar : module->globals()) {
        bool isPrivate = false;

        /*if (gvar.hasName()) {
            if (gvar.hasPrivateLinkage()) {
                isPrivate = true;
            }
            std::string replacedName = gvar.getName().str();
            std::replace(replacedName.begin(), replacedName.end(), '.', '_');
            gvarName = replacedName;
        } else {
            gvarName = getGvarName();
        }*/

        if (gvar.hasPrivateLinkage()) {
            isPrivate = true;
        }
        std::string gvarName = gvar.getName().str();
        std::replace(gvarName.begin(), gvarName.end(), '.', '_');

        std::string value = "";
        if (gvar.hasInitializer()) {
            value = getValue(gvar.getInitializer());
        }

        llvm::PointerType* PI = llvm::cast<llvm::PointerType>(gvar.getType());
        globalVars.push_back(std::make_unique<GlobalValue>(gvarName, value, Type::getType(PI->getElementType())));
        globalVars.at(globalVars.size() - 1)->getType()->isStatic = isPrivate;
        globalRefs[&gvar] = std::make_unique<RefExpr>(globalVars.at(globalVars.size() - 1).get());
    }
}

std::string Program::getStructVarName() {
    std::string varName = "structVar";
    varName += std::to_string(structVarCount);
    structVarCount++;

    return varName;
}

/*std::string Program::getGvarName() {
    std::string varName = "gvar";
    varName += std::to_string(gvarCount);
    structVarCount++;

    return varName;
}*/

std::string Program::getValue(const llvm::Constant* val) const {
    if (llvm::PointerType* PT = llvm::dyn_cast<llvm::PointerType>(val->getType())) {
        if (llvm::GlobalVariable* GV = llvm::dyn_cast<llvm::GlobalVariable>(val->getOperand(0))) {
            std::string replacedName = GV->getName().str();
            std::replace(replacedName.begin(), replacedName.end(), '.', '_');
            return replacedName;
        } else {
            return "&" + val->getName().str();
        }
    }

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

    if (llvm::ConstantStruct* CS = llvm::dyn_cast<llvm::ConstantStruct>(val)) {
        std::string value = "{";
        bool first = true;
        for (unsigned i = 0; i < CS->getNumOperands(); i++) {
            if (!first) {
                value += ", ";
            }
            first = false;

            value += getValue(val->getOperand(i));
        }

        return value + "}";
    }

    return "";
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

    if (hasVarArg) {
        llvm::outs() << "#include <stdarg.h>\n\n";
    }

    if (!declarations.empty()) {
        llvm::outs() << "//Function declarations\n";
        for (const auto& func : declarations) {
            func->print();
        }
        llvm::outs() << "\n";
    }

    if (!structs.empty()) {
        llvm::outs() << "//Struct declarations\n";
        for (auto& strct : structs) {
            llvm::outs() << "struct " << strct->name << ";\n";
        }
        llvm::outs() << "\n";
        llvm::outs() << "//Struct definitions\n";
        for (auto& strct : structs) {
            if (!strct->isPrinted) {
                printStruct(strct.get());
            }
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
        llvm::outs() << "//Global variable definitions\n";
        for (auto& gvar : globalVars) {
            llvm::outs() << gvar->toString();
            gvar->init = true;
            llvm::outs() << "\n";
        }
        llvm::outs() << "\n";
    }

    if (!functions.empty()) {
        llvm::outs() << "//Functions\n";
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
                    llvm::outs() << "\n";
                }
            }
        }
    }
    if (!strct->isPrinted) {
        strct->print();
        strct->isPrinted = true;
    }
    llvm::outs() << "\n";
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
                    file << "\n";
                }
            }
        }
    }
    if (!strct->isPrinted) {
        file << strct->toString();
        strct->isPrinted = true;
    }
    file << "\n";
}

void Program::saveFile(const std::string& fileName) {
    unsetAllInit();

    std::ofstream file;
    file.open(fileName);

    if (hasVarArg) {
        file << "#include <stdarg.h>\n\n";
    }

    if (!declarations.empty()) {
        file << "//Function declarations\n";
        for (const auto& func : declarations) {
            func->saveFile(file);
        }
        file << "\n";
    }

    if (!structs.empty()) {
        file << "//Struct declarations\n";
        for (auto& strct : structs) {
            file << "struct " << strct->name << ";\n";
        }
        file << "\n";
        file << "//Struct definitions\n";
        for (auto& strct : structs) {
            if (!strct->isPrinted) {
                saveStruct(strct.get(), file);
            }
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
        file << "//Global variable definitions\n";
        for (auto& gvar : globalVars) {
            file << gvar->toString();
            gvar->init = true;
            file << "\n";
        }
        file << "\n";
    }

    file << "//Functions\n";
    for (const auto& func : functions) {
        func->saveFile(file);
    }

    file.close();

    llvm::outs() << "Translated program successfuly saved into " << fileName << "\n";
}

Struct* Program::getStruct(const std::string& name) const {
    for (const auto& strct : structs) {
        if (strct->name == name) {
            return strct.get();
        }
    }

    return nullptr;
}

RefExpr* Program::getGlobalVar(llvm::Value* val) const {
    llvm::GlobalVariable* GV = llvm::cast<llvm::GlobalVariable>(val);
    return globalRefs[GV].get();
}

void Program::addDeclaration(llvm::Function* func) {
    declarations.push_back(std::make_unique<Func>(func, this, true));
}
