#include "Block.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"

#include "Func.h"
#include "Type.h"
#include "Expr/BinaryExpr.h"
#include "Expr/UnaryExpr.h"

#include <utility>
#include <cstdint>
#include <string>
#include <set>
#include <iostream>
#include <fstream>
#include <regex>

using CaseHandle = const llvm::SwitchInst::CaseHandleImpl<const llvm::SwitchInst, const llvm::ConstantInt, const llvm::BasicBlock>*;

const std::set<std::string> C_FUNCTIONS = {"memcpy", "memmove", "memset", "sqrt", "powi", "sin", "cos", "pow", "exp", "exp2", "log", "log10", "log2",
                                             "fma", "fabs", "minnum", "maxnum", "minimum", "maximum", "copysign", "floor", "ceil", "trunc", "rint", "nearbyint", "round"};

Block::Block(const std::string &blockName, const llvm::BasicBlock* block, Func* func)
    : blockName(blockName),
      block(block),
      func(func) { }

void Block::parseLLVMBlock() {
    for (const auto& ins : *block) {
        parseLLVMInstruction(ins);
    }
}

void Block::print() {
    unsetAllInit();

    for (const auto expr : abstractSyntaxTree) {
        if (auto val = dynamic_cast<Value*>(expr)) {
            llvm::outs() << "    ";
            if (!val->init) {
                val->type->print();
                llvm::outs() << " ";
                expr->print();

                if (auto AT = dynamic_cast<ArrayType*>(val->type.get())) {
                    AT->printSize();
                }

                if (auto PT = dynamic_cast<PointerType*>(val->type.get())) {
                    if (PT->isFuncPointer) {
                        llvm::outs() << PT->params;
                    }
                }

                llvm::outs() << ";\n";
                val->init = true;
            }
        } else if (auto CE = dynamic_cast<CallExpr*>(expr)) {
            if (!CE->isUsed) {
                llvm::outs() << "    ";
                expr->print();
                llvm::outs() << ";\n";
            }
        } else {
            llvm::outs() << "    ";
            expr->print();
            llvm::outs() << "\n";
        }
        llvm::outs().flush();
    }
}

void Block::saveFile(std::ofstream& file) {
    unsetAllInit();

    for (const auto expr : abstractSyntaxTree) {
        if (auto val = dynamic_cast<Value*>(expr)) {
            file << "    ";
            if (!val->init) {
                file << val->type->toString();
                file << " ";
                file << expr->toString();

                if (auto AT = dynamic_cast<ArrayType*>(val->type.get())) {
                    file << AT->sizeToString();
                }

                if (auto PT = dynamic_cast<PointerType*>(val->type.get())) {
                    if (PT->isFuncPointer) {
                        file << PT->params;
                    }
                }

                file << ";\n";
                val->init = true;
            }
        } else if (auto CE = dynamic_cast<CallExpr*>(expr)) {
            if (!CE->isUsed) {
                file << "    ";
                file << expr->toString();
                file << ";\n";
            }
        } else {
            file << "    ";
            file << expr->toString();
            file << "\n";
        }
    }
}

void Block::parseAllocaInstruction(const llvm::Instruction& ins) {
    const auto allocaInst = llvm::cast<const llvm::AllocaInst>(&ins);

    func->valueMap[&ins] = std::make_unique<Value>(func->getVarName(), Type::getType(allocaInst->getAllocatedType()));
    func->createExpr(&ins, std::make_unique<RefExpr>(func->valueMap[&ins].get()));
    abstractSyntaxTree.push_back(func->valueMap[&ins].get());
}

void Block::parseLoadInstruction(const llvm::Instruction& ins) {
    if (func->exprMap.find(ins.getOperand(0)) == func->exprMap.end()) {
        func->createExpr(&ins, std::make_unique<DerefExpr>(func->getGlobalVar(ins.getOperand(0))));
    } else {
        func->createExpr(&ins, std::make_unique<DerefExpr>(func->exprMap[ins.getOperand(0)].get()));
    }
}

void Block::parseStoreInstruction(const llvm::Instruction& ins) {
    auto type = std::move(Type::getType(ins.getOperand(0)->getType()));
    if (auto PT = dynamic_cast<PointerType*>(type.get())) {
        if (llvm::Function* function = llvm::dyn_cast<llvm::Function>(ins.getOperand(0))) {
            func->createExpr(ins.getOperand(0), std::make_unique<Value>("&" + function->getName().str(), std::make_unique<VoidType>()));
        }
    }
    if (func->getExpr(ins.getOperand(0)) == nullptr) {
        createConstantValue(ins.getOperand(0));
    }
    Expr* val0 = func->getExpr(ins.getOperand(0));

    if (auto CE = dynamic_cast<CallExpr*>(val0)) {
        CE->isUsed = true;
    }

    Expr* val1 = func->getExpr(ins.getOperand(1));
    if (derefs.find(val1) == derefs.end()) {
        derefs[val1] = std::make_unique<DerefExpr>(val1);
    }
    func->createExpr(&ins, std::make_unique<EqualsExpr>(derefs[val1].get(), val0));

    abstractSyntaxTree.push_back(func->exprMap[llvm::cast<const llvm::Value>(&ins)].get());
}

void Block::parseBinaryInstruction(const llvm::Instruction& ins) {
    if (func->getExpr(ins.getOperand(0)) == nullptr) {
        createConstantValue(ins.getOperand(0));
    }
    Expr* val0 = func->getExpr(ins.getOperand(0));

    if (func->getExpr(ins.getOperand(1)) == nullptr) {
        createConstantValue(ins.getOperand(1));
    }
    Expr* val1 = func->getExpr(ins.getOperand(1));

    unsigned opcode = ins.getOpcode();
    if (opcode == llvm::Instruction::Add || opcode == llvm::Instruction::FAdd) {
        func->createExpr(&ins, std::make_unique<AddExpr>(val0, val1));
    } else if (opcode == llvm::Instruction::Sub || opcode == llvm::Instruction::FSub) {
        func->createExpr(&ins, std::make_unique<SubExpr>(val0, val1));
    } else if (opcode == llvm::Instruction::Mul || opcode == llvm::Instruction::FMul) {
        func->createExpr(&ins, std::make_unique<MulExpr>(val0, val1));
    } else if (opcode == llvm::Instruction::SDiv || opcode == llvm::Instruction::UDiv || opcode == llvm::Instruction::FDiv) {
        func->createExpr(&ins, std::make_unique<DivExpr>(val0, val1));
    } else if (opcode == llvm::Instruction::SRem || opcode == llvm::Instruction::URem || opcode == llvm::Instruction::FRem) {
        func->createExpr(&ins, std::make_unique<RemExpr>(val0, val1));
    } else if (opcode == llvm::Instruction::And) {
        func->createExpr(&ins, std::make_unique<AndExpr>(val0, val1));
    } else if (opcode == llvm::Instruction::Or) {
        func->createExpr(&ins, std::make_unique<OrExpr>(val0, val1));
    } else if (opcode == llvm::Instruction::Xor) {
        func->createExpr(&ins, std::make_unique<XorExpr>(val0, val1));
    }
}

void Block::parseCmpInstruction(const llvm::Instruction& ins) {
    if (func->getExpr(ins.getOperand(0)) == nullptr) {
        createConstantValue(ins.getOperand(0));
    }
    Expr* val0 = func->getExpr(ins.getOperand(0));

    if (func->getExpr(ins.getOperand(1)) == nullptr) {
        createConstantValue(ins.getOperand(1));
    }
    Expr* val1 = func->getExpr(ins.getOperand(1));

    if (auto CE = dynamic_cast<CallExpr*>(val0)) {
        CE->isUsed = true;
    }
    if (auto CE = dynamic_cast<CallExpr*>(val1)) {
        CE->isUsed = true;
    }

    auto cmpInst = llvm::cast<const llvm::CmpInst>(&ins);

    if (cmpInst->getPredicate() == llvm::CmpInst::ICMP_EQ || cmpInst->getPredicate() == llvm::CmpInst::FCMP_OEQ) {
        func->createExpr(&ins, std::make_unique<CmpExpr>(val0, val1, "=="));
    } else if (cmpInst->getPredicate() == llvm::CmpInst::ICMP_NE || cmpInst->getPredicate() == llvm::CmpInst::FCMP_ONE) {
        func->createExpr(&ins, std::make_unique<CmpExpr>(val0, val1, "!="));
    } else if (cmpInst->getPredicate() == llvm::CmpInst::ICMP_UGT || cmpInst->getPredicate() == llvm::CmpInst::ICMP_SGT
               || cmpInst->getPredicate() == llvm::CmpInst::FCMP_OGT) {
        func->createExpr(&ins, std::make_unique<CmpExpr>(val0, val1, ">"));
    } else if (cmpInst->getPredicate() == llvm::CmpInst::ICMP_UGE || cmpInst->getPredicate() == llvm::CmpInst::ICMP_SGE
               || cmpInst->getPredicate() == llvm::CmpInst::FCMP_OGE) {
        func->createExpr(&ins, std::make_unique<CmpExpr>(val0, val1, ">="));
    } else if (cmpInst->getPredicate() == llvm::CmpInst::ICMP_ULT || cmpInst->getPredicate() == llvm::CmpInst::ICMP_SLT
               || cmpInst->getPredicate() == llvm::CmpInst::FCMP_OLT) {
        func->createExpr(&ins, std::make_unique<CmpExpr>(val0, val1, "<"));
    } else if (cmpInst->getPredicate() == llvm::CmpInst::ICMP_ULE || cmpInst->getPredicate() == llvm::CmpInst::ICMP_SLE
               || cmpInst->getPredicate() == llvm::CmpInst::FCMP_OLE) {
        func->createExpr(&ins, std::make_unique<CmpExpr>(val0, val1, "<="));
    } else if (cmpInst->getPredicate() == llvm::CmpInst::FCMP_FALSE) {
        func->createExpr(&ins, std::make_unique<Value>("0", std::make_unique<IntegerType>("int", false)));
    } else if (cmpInst->getPredicate() == llvm::CmpInst::FCMP_TRUE) {
        func->createExpr(&ins, std::make_unique<Value>("1", std::make_unique<IntegerType>("int", false)));
    }
}

void Block::parseBrInstruction(const llvm::Instruction& ins) {
    if (ins.getNumOperands() == 1) {
        std::string trueBlock = func->getBlockName((llvm::BasicBlock*)ins.getOperand(0));
        func->createExpr(&ins, std::make_unique<IfExpr>(trueBlock));

        abstractSyntaxTree.push_back(func->exprMap[llvm::cast<const llvm::Value>(&ins)].get());
        return;
    }

    Expr* cmp = func->exprMap[ins.getOperand(0)].get();

    std::string falseBlock = func->getBlockName((llvm::BasicBlock*)ins.getOperand(1));
    std::string trueBlock = func->getBlockName((llvm::BasicBlock*)ins.getOperand(2));

    func->createExpr(&ins, std::make_unique<IfExpr>(cmp, trueBlock, falseBlock));

    abstractSyntaxTree.push_back(func->exprMap[llvm::cast<const llvm::Value>(&ins)].get());
}

void Block::parseRetInstruction(const llvm::Instruction& ins) {
    if (ins.getNumOperands() == 0) {
        func->createExpr(&ins, std::make_unique<RetExpr>());
    } else {
        if (func->getExpr(ins.getOperand(0)) == nullptr) {
            createConstantValue(ins.getOperand(0));
        }
        Expr* expr = func->getExpr(ins.getOperand(0));
        if (auto CE = dynamic_cast<CallExpr*>(expr)) {
            CE->isUsed = true;
        }

        func->createExpr(&ins, std::make_unique<RetExpr>(expr));
    }

    abstractSyntaxTree.push_back(func->exprMap[llvm::cast<const llvm::Value>(&ins)].get());
}

void Block::parseSwitchInstruction(const llvm::Instruction& ins) {
    std::map<int, std::string> cases;
    Expr* cmp = func->getExpr(ins.getOperand(0));
    std::string def = func->getBlockName((llvm::BasicBlock*)ins.getOperand(1));
    const llvm::SwitchInst* switchIns = llvm::cast<const llvm::SwitchInst>(&ins);

    for (const auto& switchCase : switchIns->cases()) {
        CaseHandle caseHandle = static_cast<CaseHandle>(&switchCase);
        cases[caseHandle->getCaseValue()->getSExtValue()] = func->getBlockName(caseHandle->getCaseSuccessor());
    }

    func->createExpr(&ins, std::make_unique<SwitchExpr>(cmp, def, cases));

    abstractSyntaxTree.push_back(func->exprMap[llvm::cast<const llvm::Value>(&ins)].get());
}

void Block::parseAsmInst(const llvm::Instruction& ins) {
    std::string inst;

    switch(ins.getOpcode()) {
    case llvm::Instruction::Unreachable:
        inst = "int3";
        break;
    case llvm::Instruction::Fence:
        inst = "fence";
        break;
    default:
        break;
    }
    func->createExpr(&ins, std::make_unique<AsmExpr>(inst));

    abstractSyntaxTree.push_back(func->exprMap[llvm::cast<const llvm::Value>(&ins)].get());
}

void Block::parseShiftInstruction(const llvm::Instruction& ins) {
    if (func->getExpr(ins.getOperand(0)) == nullptr) {
        createConstantValue(ins.getOperand(0));
    }
    Expr* val0 = func->getExpr(ins.getOperand(0));

    if (func->getExpr(ins.getOperand(1)) == nullptr) {
        createConstantValue(ins.getOperand(1));
    }
    Expr* val1 = func->getExpr(ins.getOperand(1));

    switch (ins.getOpcode()) {
    case llvm::Instruction::Shl:
        func->createExpr(&ins, std::make_unique<ShlExpr>(val0, val1));
        break;
    case llvm::Instruction::LShr:
        func->createExpr(&ins, std::make_unique<LshrExpr>(val0, val1, false));
        break;
    case llvm::Instruction::AShr:
        func->createExpr(&ins, std::make_unique<AshrExpr>(val0, val1));
        break;
    }
}

void Block::parseCallInstruction(const llvm::Instruction& ins) {
    const llvm::CallInst* callInst = llvm::cast<const llvm::CallInst>(&ins);
    std::string funcName;
    std::vector<Expr*> params;
    std::unique_ptr<Type> type = nullptr;

    if (callInst->getCalledFunction() != nullptr) {
        funcName = callInst->getCalledFunction()->getName().str();
        type = Type::getType(callInst->getCalledFunction()->getReturnType());
        if (funcName.compare("llvm.dbg.declare") == 0) {
            setMetadataInfo(callInst);
            return;
        }
        if (funcName.compare("llvm.trap") == 0 || funcName.compare("llvm.debugtrap") == 0) {
            func->createExpr(&ins, std::make_unique<AsmExpr>("int3"));
            abstractSyntaxTree.push_back(func->exprMap[llvm::cast<const llvm::Value>(&ins)].get());
            return;
        }
        if (funcName.substr(0,4).compare("llvm") == 0) {
            funcName = getCFunc(funcName);
            if (!isCFunc(funcName)) {
                func->addDeclaration(callInst->getCalledFunction());
                return;
            }
        }
    } else {
        llvm::PointerType* PT = llvm::cast<llvm::PointerType>(callInst->getCalledValue()->getType());
        llvm::FunctionType* FT = llvm::cast<llvm::FunctionType>(PT->getElementType());
        funcName = func->getExpr(callInst->getCalledValue())->toString();
        type = Type::getType(FT->getReturnType());
    }

    for (const auto& param : callInst->arg_operands()) {
        if (func->getExpr(param) == nullptr) {
            createConstantValue(param);
        }
        params.push_back(func->getExpr(param));
    }

    func->callExprMap[&ins] = std::make_unique<CallExpr>(funcName, params, type->clone());
    func->createExpr(&ins, std::make_unique<Value>(func->getVarName(), type->clone()));
    func->callValueMap[&ins] = std::make_unique<EqualsExpr>(func->getExpr(&ins), func->callExprMap[&ins].get());

    abstractSyntaxTree.push_back(func->getExpr(&ins));
    abstractSyntaxTree.push_back(func->callValueMap[&ins].get());
}

void Block::parseCastInstruction(const llvm::Instruction& ins) {
    if (func->getExpr(ins.getOperand(0)) == nullptr) {
        createConstantValue(ins.getOperand(0));
    }
    Expr* expr = func->getExpr(ins.getOperand(0));

    const llvm::CastInst* CI = llvm::cast<const llvm::CastInst>(&ins);

    func->createExpr(&ins, std::make_unique<CastExpr>(expr, std::move(Type::getType(CI->getDestTy()))));
}

void Block::parseSelectInstruction(const llvm::Instruction& ins) {
    const llvm::SelectInst* SI = llvm::cast<const llvm::SelectInst>(&ins);
    Expr* cond = func->getExpr(SI->getCondition());

    if (func->getExpr(ins.getOperand(1)) == nullptr) {
        createConstantValue(ins.getOperand(1));
    }
    Expr* val0 = func->getExpr(ins.getOperand(1));

    if (func->getExpr(ins.getOperand(2)) == nullptr) {
        createConstantValue(ins.getOperand(2));
    }
    Expr* val1 = func->getExpr(ins.getOperand(2));

    func->createExpr(&ins, std::make_unique<SelectExpr>(cond, val0, val1));
}

void Block::parseGepInstruction(const llvm::Instruction& ins) {
    const llvm::GetElementPtrInst* gepInst = llvm::cast<llvm::GetElementPtrInst>(&ins);

    Expr* expr = func->getExpr(gepInst->getOperand(0));
    auto gepExpr = std::make_unique<GepExpr>(expr, Type::getType(gepInst->getType()));

    std::string indexValue;
    llvm::Type* prevType = gepInst->getOperand(0)->getType();

    bool isStruct = false;
    llvm::PointerType* PT = llvm::cast<llvm::PointerType>(gepInst->getOperand(0)->getType());
    if (PT->getElementType()->isStructTy()) {
        isStruct = true;
        llvm::StructType* ST = llvm::cast<llvm::StructType>(PT->getElementType());
        std::string structName = ST->getName().str().erase(0, 7);
        std::string varName = func->getExpr(gepInst->getOperand(0))->toString();

        if (varName.at(0) == '&') {
            varName = func->getExpr(gepInst->getOperand(0))->toString().erase(0,2);
            varName = varName.erase(varName.length() - 1, varName.length());
        }

        if (structName.compare("__va_list_tag") == 0) {
            gepExpr->addArg(std::make_unique<VoidType>(), "va_list");  //TODO
        } else {
            structElements[&ins] = std::make_unique<StructElement>(func->getStruct(structName), varName, llvm::cast<llvm::ConstantInt>(gepInst->getOperand(2))->getSExtValue());
            refs[structElements[&ins].get()] = std::make_unique<RefExpr>(structElements[&ins].get());
            gepExpr = std::make_unique<GepExpr>(refs[structElements[&ins].get()].get(), Type::getType(gepInst->getType()));
            //gepExpr->addArg(std::make_unique<VoidType>(), "&((" + varName + ")." + func->getStruct(structName)->items[llvm::cast<llvm::ConstantInt>(gepInst->getOperand(2))->getSExtValue()].second + ")");
        }
    }

    for (auto it = llvm::gep_type_begin(gepInst); it != llvm::gep_type_end(gepInst); it++) {
        if (isStruct) {
            std::advance(it, 2);
            if (it == llvm::gep_type_end(gepInst)) {
                break;
            }
        }

        if (auto CI = llvm::dyn_cast<llvm::ConstantInt>(it.getOperand())) {
            indexValue = std::to_string(CI->getSExtValue());
        } else {
            indexValue = func->getExpr(it.getOperand())->toString();
        }


        if (prevType->isArrayTy()) {
            unsigned int size = prevType->getArrayNumElements();
            gepExpr->addArg(Type::getType(prevType), indexValue);
        } else {
            gepExpr->addArg(std::make_unique<PointerType>(Type::getType(prevType)), indexValue);
        }
        prevType = it.getIndexedType();
    }

    func->createExpr(&ins, std::move(gepExpr));
}

void Block::parseLLVMInstruction(const llvm::Instruction& ins) {
    switch (ins.getOpcode()) {
    case llvm::Instruction::Add:
    case llvm::Instruction::FAdd:
    case llvm::Instruction::Sub:
    case llvm::Instruction::FSub:
    case llvm::Instruction::Mul:
    case llvm::Instruction::FMul:
    case llvm::Instruction::UDiv:
    case llvm::Instruction::FDiv:
    case llvm::Instruction::SDiv:
    case llvm::Instruction::URem:
    case llvm::Instruction::FRem:
    case llvm::Instruction::SRem:
    case llvm::Instruction::And:
    case llvm::Instruction::Or:
    case llvm::Instruction::Xor:
        parseBinaryInstruction(ins);
        break;
    case llvm::Instruction::Alloca:
        parseAllocaInstruction(ins);
        break;
    case llvm::Instruction::Load:
        parseLoadInstruction(ins);
        break;
    case llvm::Instruction::Store:
        parseStoreInstruction(ins);
        break;
    case llvm::Instruction::ICmp:
    case llvm::Instruction::FCmp:
        parseCmpInstruction(ins);
        break;
    case llvm::Instruction::Br:
        parseBrInstruction(ins);
        break;
    case llvm::Instruction::Ret:
        parseRetInstruction(ins);
        break;
    case llvm::Instruction::Switch:
        parseSwitchInstruction(ins);
        break;
    case llvm::Instruction::Unreachable:
    case llvm::Instruction::Fence:
        parseAsmInst(ins);
        break;
    case llvm::Instruction::Shl:
    case llvm::Instruction::LShr:
    case llvm::Instruction::AShr:
        parseShiftInstruction(ins);
        break;
    case llvm::Instruction::Call:
        parseCallInstruction(ins);
        break;
    case llvm::Instruction::SExt:
    case llvm::Instruction::ZExt:
    case llvm::Instruction::FPToSI:
    case llvm::Instruction::SIToFP:
    case llvm::Instruction::FPTrunc:
    case llvm::Instruction::FPExt:
    case llvm::Instruction::FPToUI:
    case llvm::Instruction::UIToFP:
    case llvm::Instruction::PtrToInt:
    case llvm::Instruction::IntToPtr:
    case llvm::Instruction::Trunc:
    case llvm::Instruction::BitCast:
        parseCastInstruction(ins);
        break;
    case llvm::Instruction::Select:
        parseSelectInstruction(ins);
        break;
    case llvm::Instruction::GetElementPtr:
        parseGepInstruction(ins);
        break;
    case llvm::Instruction::PHI:
        throw std::invalid_argument("Instruction \"phi\" is not supported!");
        break;
    default:
        throw std::invalid_argument("Instruction not supported!");
        break;
    }
}

void Block::setMetadataInfo(const llvm::CallInst* ins) {
    llvm::Metadata* md = llvm::dyn_cast<llvm::MetadataAsValue>(ins->getOperand(0))->getMetadata();
    llvm::Value* referredVal = llvm::cast<llvm::ValueAsMetadata>(md)->getValue();

    if (Value* variable = func->valueMap[referredVal].get()) {
        llvm::Metadata* varMD = llvm::dyn_cast<llvm::MetadataAsValue>(ins->getOperand(1))->getMetadata();
        llvm::DILocalVariable* localVar = llvm::dyn_cast<llvm::DILocalVariable>(varMD);
        llvm::DIBasicType* type = llvm::dyn_cast<llvm::DIBasicType>(localVar->getType());

        std::regex varName("var[0-9]+");
        std::regex constGlobalVarName("ConstGlobalVar_.+");
        if (!std::regex_match(localVar->getName().str(), varName) && !std::regex_match(localVar->getName().str(), constGlobalVarName)) {
            variable->valueName = localVar->getName();
        }

        if (type && type->getName().str().compare(0, 8, "unsigned") == 0) {
            if (IntegerType* IT = dynamic_cast<IntegerType*>(variable->type.get())) {
                IT->unsignedType = true;
            }
        }
    }
}

void Block::unsetAllInit() {
    for (auto expr : abstractSyntaxTree) {
        if (Value* val = dynamic_cast<Value*>(expr)) {
            val->init = false;
        }
    }
}

void Block::createConstantValue(llvm::Value* val) {
    if (llvm::ConstantPointerNull* CPN = llvm::dyn_cast<llvm::ConstantPointerNull>(val)) {
        func->createExpr(val, std::make_unique<Value>("0", std::make_unique<VoidType>()));
    }
    if (llvm::ConstantInt* CI = llvm::dyn_cast<llvm::ConstantInt>(val)) {
        func->createExpr(val, std::make_unique<Value>(std::to_string(CI->getSExtValue()), std::make_unique<IntType>(false)));
    }
    if (llvm::ConstantFP* CFP = llvm::dyn_cast<llvm::ConstantFP>(val)) {
        func->createExpr(val, std::make_unique<Value>(std::to_string(CFP->getValueAPF().convertToDouble()), std::make_unique<IntType>(false)));
    }
    if (llvm::ConstantExpr* CE = llvm::dyn_cast<llvm::ConstantExpr>(val)) {
        parseConstantGep(CE);
    }
}

void Block::parseConstantGep(llvm::ConstantExpr *expr) const {
    Expr* gvar = func->getGlobalVar(expr->getOperand(0));
    auto gepExpr = std::make_unique<GepExpr>(gvar, Type::getType(expr->getType()));

    std::string indexValue;
    llvm::Type* prevType = expr->getOperand(0)->getType();

    bool isStruct = false;
    llvm::PointerType* PT = llvm::cast<llvm::PointerType>(expr->getOperand(0)->getType());
    if (PT->getElementType()->isStructTy()) {
        isStruct = true;
        llvm::StructType* ST = llvm::cast<llvm::StructType>(PT->getElementType());
        std::string structName = ST->getName().str().erase(0, 7);
        std::string varName = func->getExpr(expr->getOperand(0))->toString();

        if (varName.at(0) == '&') {
            varName = func->getExpr(expr->getOperand(0))->toString().erase(0,2);
            varName = varName.erase(varName.length() - 1, varName.length());
        }

        gepExpr->addArg(std::make_unique<VoidType>(), "&((" + varName + ")." + func->getStruct(structName)->items[llvm::cast<llvm::ConstantInt>(expr->getOperand(2))->getSExtValue()].second + ")");
    }

    for (auto it = llvm::gep_type_begin(expr); it != llvm::gep_type_end(expr); it++) {
        if (isStruct) {
            std::advance(it, 2);
            if (it == llvm::gep_type_end(expr)) {
                break;
            }
        }

        if (auto CI = llvm::dyn_cast<llvm::ConstantInt>(it.getOperand())) {
            indexValue = std::to_string(CI->getSExtValue());
        } else {
            indexValue = func->getExpr(it.getOperand())->toString();
        }


        if (prevType->isArrayTy()) {
            unsigned int size = prevType->getArrayNumElements();
            gepExpr->addArg(std::move(Type::getType(prevType)), indexValue);
        } else {
            gepExpr->addArg(std::make_unique<PointerType>(Type::getType(prevType)), indexValue);
        }
        prevType = it.getIndexedType();
    }

    func->createExpr(expr, std::move(gepExpr));

}

bool Block::isCFunc(const std::string& func) const {
    return C_FUNCTIONS.find(func) != C_FUNCTIONS.end();
}

std::string Block::getCFunc(const std::string& func) const {
    std::regex function("llvm\\.(\\w+)(\\..+){0,1}");
    std::smatch match;
    if (std::regex_search(func, match, function)) {
        return match[1].str();
    }

    return "";
}
