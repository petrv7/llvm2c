#include "Block.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/InlineAsm.h"

#include "Func.h"
#include "Type/Type.h"
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
                                           "fma", "fabs", "minnum", "maxnum", "minimum", "maximum", "copysign", "floor", "ceil", "trunc", "rint", "nearbyint",
                                           "round", "va_start", "va_end"};

Block::Block(const std::string &blockName, const llvm::BasicBlock* block, Func* func)
    : block(block),
      func(func),
      blockName(blockName) { }

void Block::parseLLVMBlock() {
    for (const auto& ins : *block) {
        parseLLVMInstruction(ins, false, nullptr);
    }
}

void Block::print() {
    unsetAllInit();

    for (const auto expr : abstractSyntaxTree) {
        if (auto val = dynamic_cast<Value*>(expr)) {
            llvm::outs() << "    ";
            if (!val->init) {
                val->getType()->print();
                llvm::outs() << " ";
                expr->print();

                if (auto AT = dynamic_cast<ArrayType*>(val->getType())) {
                    AT->printSize();
                }

                if (auto PT = dynamic_cast<PointerType*>(val->getType())) {
                    if (PT->isFuncPointer) {
                        llvm::outs() << PT->params;
                    }
                }

                llvm::outs() << ";\n";
                val->init = true;
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
                file << val->getType()->toString();
                file << " ";
                file << expr->toString();

                if (auto AT = dynamic_cast<ArrayType*>(val->getType())) {
                    file << AT->sizeToString();
                }

                if (auto PT = dynamic_cast<PointerType*>(val->getType())) {
                    if (PT->isFuncPointer) {
                        file << PT->params;
                    }
                }

                file << ";\n";
                val->init = true;
            }
        } else {
            file << "    ";
            file << expr->toString();
            file << "\n";
        }
    }
}

void Block::parseAllocaInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val) {
    const auto allocaInst = llvm::cast<const llvm::AllocaInst>(&ins);

    const llvm::Value* value = isConstExpr ? val : &ins;

    func->valueMap[value] = std::make_unique<Value>(func->getVarName(), Type::getType(allocaInst->getAllocatedType()));
    func->createExpr(value, std::make_unique<RefExpr>(func->valueMap[&ins].get()));

    if (!isConstExpr) {
        abstractSyntaxTree.push_back(func->valueMap[&ins].get());
    }
}

void Block::parseLoadInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val) {
    if (!func->getExpr(ins.getOperand(0))) {
        createConstantValue(ins.getOperand(0));
    }

    if (!isConstExpr) {
        auto test = func->getExpr(ins.getOperand(0));
        func->createExpr(&ins, std::make_unique<DerefExpr>(func->getExpr(ins.getOperand(0))));
    } else {
        func->createExpr(val, std::make_unique<DerefExpr>(func->getExpr(ins.getOperand(0))));
    }
}

void Block::parseStoreInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val) {
    auto type = Type::getType(ins.getOperand(0)->getType());
    if (auto PT = dynamic_cast<PointerType*>(type.get())) {
        if (llvm::Function* function = llvm::dyn_cast<llvm::Function>(ins.getOperand(0))) {
            if (!func->getExpr(ins.getOperand(0))) {
                func->createExpr(ins.getOperand(0), std::make_unique<Value>("&" + function->getName().str(), std::make_unique<VoidType>()));
            }
        }
    }

    if (llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(ins.getOperand(0))) {
        if (CI->getCalledFunction()) {
            if (CI->getCalledFunction()->getName().str().compare("llvm.stacksave") == 0) {
                return;
            }
        }
    }

    if (func->getExpr(ins.getOperand(0)) == nullptr) {
        createConstantValue(ins.getOperand(0));
    }
    Expr* val0 = func->getExpr(ins.getOperand(0));

    if (func->getExpr(ins.getOperand(1)) == nullptr) {
        createConstantValue(ins.getOperand(1));
    }
    Expr* val1 = func->getExpr(ins.getOperand(1));
    if (derefs.find(val1) == derefs.end()) {
        derefs[val1] = std::make_unique<DerefExpr>(val1);
    }

    if (derefs[val1]->toString() == "var99") {
        llvm::outs().flush();
    }

    if (!isConstExpr) {
        func->createExpr(&ins, std::make_unique<EqualsExpr>(derefs[val1].get(), val0));
        abstractSyntaxTree.push_back(func->getExpr(&ins));
    } else {
        func->createExpr(val, std::make_unique<EqualsExpr>(derefs[val1].get(), val0));
    }
}

void Block::parseBinaryInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val) {
    if (func->getExpr(ins.getOperand(0)) == nullptr) {
        createConstantValue(ins.getOperand(0));
    }
    Expr* val0 = func->getExpr(ins.getOperand(0));

    if (func->getExpr(ins.getOperand(1)) == nullptr) {
        createConstantValue(ins.getOperand(1));
    }
    Expr* val1 = func->getExpr(ins.getOperand(1));

    const llvm::Value* value = isConstExpr ? val : &ins;

    switch (ins.getOpcode()) {
    case llvm::Instruction::Add:
    case llvm::Instruction::FAdd:
        func->createExpr(value, std::make_unique<AddExpr>(val0, val1));
        break;
    case llvm::Instruction::Sub:
    case llvm::Instruction::FSub:
        func->createExpr(value, std::make_unique<SubExpr>(val0, val1));
        break;
    case llvm::Instruction::Mul:
    case llvm::Instruction::FMul:
        func->createExpr(value, std::make_unique<MulExpr>(val0, val1));
        break;
    case llvm::Instruction::SDiv:
    case llvm::Instruction::UDiv:
    case llvm::Instruction::FDiv:
        func->createExpr(value, std::make_unique<DivExpr>(val0, val1));
        break;
    case llvm::Instruction::SRem:
    case llvm::Instruction::URem:
    case llvm::Instruction::FRem:
        func->createExpr(value, std::make_unique<RemExpr>(val0, val1));
        break;
    case llvm::Instruction::And:
        func->createExpr(value, std::make_unique<AndExpr>(val0, val1));
        break;
    case llvm::Instruction::Or:
        func->createExpr(value, std::make_unique<OrExpr>(val0, val1));
        break;
    case llvm::Instruction::Xor:
        func->createExpr(value, std::make_unique<XorExpr>(val0, val1));
        break;
    }
}

void Block::parseCmpInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val) {
    if (func->getExpr(ins.getOperand(0)) == nullptr) {
        createConstantValue(ins.getOperand(0));
    }
    Expr* val0 = func->getExpr(ins.getOperand(0));

    if (func->getExpr(ins.getOperand(1)) == nullptr) {
        createConstantValue(ins.getOperand(1));
    }
    Expr* val1 = func->getExpr(ins.getOperand(1));

    if (!val1) {
        llvm::outs() << ins << "\n";
        llvm::outs().flush();
    }

    auto cmpInst = llvm::cast<const llvm::CmpInst>(&ins);
    const llvm::Value* value = isConstExpr ? val : &ins;

    switch(cmpInst->getPredicate()) {
    case llvm::CmpInst::ICMP_EQ:
    case llvm::CmpInst::FCMP_OEQ:
    case llvm::CmpInst::FCMP_UEQ:
        func->createExpr(value, std::make_unique<CmpExpr>(val0, val1, "==", false));
        break;
    case llvm::CmpInst::ICMP_NE:
    case llvm::CmpInst::FCMP_ONE:
    case llvm::CmpInst::FCMP_UNE:
        func->createExpr(value, std::make_unique<CmpExpr>(val0, val1, "!=", false));
        break;
    case llvm::CmpInst::ICMP_UGT:
    case llvm::CmpInst::ICMP_SGT:
    case llvm::CmpInst::FCMP_UGT:
    case llvm::CmpInst::FCMP_OGT:
        func->createExpr(value, std::make_unique<CmpExpr>(val0, val1, ">", false));
        break;
    case llvm::CmpInst::ICMP_UGE:
    case llvm::CmpInst::ICMP_SGE:
    case llvm::CmpInst::FCMP_OGE:
    case llvm::CmpInst::FCMP_UGE:
        func->createExpr(value, std::make_unique<CmpExpr>(val0, val1, ">=", false));
        break;
    case llvm::CmpInst::ICMP_ULT:
    case llvm::CmpInst::ICMP_SLT:
    case llvm::CmpInst::FCMP_OLT:
    case llvm::CmpInst::FCMP_ULT:
        func->createExpr(value, std::make_unique<CmpExpr>(val0, val1, "<", false));
        break;
    case llvm::CmpInst::ICMP_ULE:
    case llvm::CmpInst::ICMP_SLE:
    case llvm::CmpInst::FCMP_OLE:
    case llvm::CmpInst::FCMP_ULE:
        func->createExpr(value, std::make_unique<CmpExpr>(val0, val1, "<=", false));
        break;
    case llvm::CmpInst::FCMP_FALSE:
        func->createExpr(value, std::make_unique<Value>("0", std::make_unique<IntegerType>("int", false)));
        break;
    case llvm::CmpInst::FCMP_TRUE:
        func->createExpr(value, std::make_unique<Value>("1", std::make_unique<IntegerType>("int", false)));
        break;
    default:
        throw std::invalid_argument("FCMP ORD/UNO and BAD PREDICATE not supported!");

    }
}

void Block::parseBrInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val) {
    const llvm::Value* value = isConstExpr ? val : &ins;

    if (ins.getNumOperands() == 1) {
        std::string trueBlock = func->getBlockName((llvm::BasicBlock*)ins.getOperand(0));
        func->createExpr(value, std::make_unique<IfExpr>(trueBlock));

        if (!isConstExpr) {
            abstractSyntaxTree.push_back(func->getExpr(&ins));
        }
        return;
    }

    Expr* cmp = func->exprMap[ins.getOperand(0)].get();

    std::string falseBlock = func->getBlockName((llvm::BasicBlock*)ins.getOperand(1));
    std::string trueBlock = func->getBlockName((llvm::BasicBlock*)ins.getOperand(2));

    func->createExpr(value, std::make_unique<IfExpr>(cmp, trueBlock, falseBlock));

    if (!isConstExpr) {
        abstractSyntaxTree.push_back(func->getExpr(&ins));
    }
}

void Block::parseRetInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val) {
    const llvm::Value* value = isConstExpr ? val : &ins;

    if (ins.getNumOperands() == 0) {
        func->createExpr(value, std::make_unique<RetExpr>());
    } else {
        if (func->getExpr(ins.getOperand(0)) == nullptr) {
            createConstantValue(ins.getOperand(0));
        }
        Expr* expr = func->getExpr(ins.getOperand(0));

        func->createExpr(value, std::make_unique<RetExpr>(expr));
    }

    if (!isConstExpr) {
        abstractSyntaxTree.push_back(func->getExpr(&ins));
    }
}

void Block::parseSwitchInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val) {
    std::map<int, std::string> cases;
    Expr* cmp = func->getExpr(ins.getOperand(0));
    std::string def = func->getBlockName(llvm::cast<llvm::BasicBlock>(ins.getOperand(1)));
    const llvm::SwitchInst* switchIns = llvm::cast<const llvm::SwitchInst>(&ins);

    for (const auto& switchCase : switchIns->cases()) {
        CaseHandle caseHandle = static_cast<CaseHandle>(&switchCase);
        cases[caseHandle->getCaseValue()->getSExtValue()] = func->getBlockName(caseHandle->getCaseSuccessor());
    }

    if (!isConstExpr) {
        func->createExpr(&ins, std::make_unique<SwitchExpr>(cmp, def, cases));
        abstractSyntaxTree.push_back(func->getExpr(&ins));
    } else {
        func->createExpr(val, std::make_unique<SwitchExpr>(cmp, def, cases));
    }
}

void Block::parseAsmInst(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val) {
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

    if (!isConstExpr) {
        func->createExpr(&ins, std::make_unique<AsmExpr>(inst));
        abstractSyntaxTree.push_back(func->getExpr(&ins));
    } else {
        func->createExpr(val, std::make_unique<AsmExpr>(inst));
    }
}

void Block::parseShiftInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val) {
    if (func->getExpr(ins.getOperand(0)) == nullptr) {
        createConstantValue(ins.getOperand(0));
    }
    Expr* val0 = func->getExpr(ins.getOperand(0));

    if (func->getExpr(ins.getOperand(1)) == nullptr) {
        createConstantValue(ins.getOperand(1));
    }
    Expr* val1 = func->getExpr(ins.getOperand(1));

    const llvm::Value* value = isConstExpr ? val : &ins;

    switch (ins.getOpcode()) {
    case llvm::Instruction::Shl:
        func->createExpr(value, std::make_unique<ShlExpr>(val0, val1));
        break;
    case llvm::Instruction::LShr:
        func->createExpr(value, std::make_unique<LshrExpr>(val0, val1));
        break;
    case llvm::Instruction::AShr:
        func->createExpr(value, std::make_unique<AshrExpr>(val0, val1));
        break;
    }
}

void Block::parseCallInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val) {
    const llvm::Value* value = isConstExpr ? val : &ins;
    const llvm::CallInst* callInst = llvm::cast<const llvm::CallInst>(&ins);
    std::string funcName;
    std::vector<Expr*> params;
    std::unique_ptr<Type> type = nullptr;
    bool isFuncPointer = false;

    if (callInst->getCalledFunction()) {
        funcName = callInst->getCalledFunction()->getName().str();
        type = Type::getType(callInst->getCalledFunction()->getReturnType());

        if (funcName.compare("llvm.dbg.declare") == 0) {
            setMetadataInfo(callInst);
            return;
        }

        if (funcName.compare("llvm.trap") == 0 || funcName.compare("llvm.debugtrap") == 0) {
            func->createExpr(&ins, std::make_unique<AsmExpr>("int3"));
            abstractSyntaxTree.push_back(func->getExpr(&ins));
            return;
        }

        if (funcName.compare("llvm.stacksave") == 0 || funcName.compare("llvm.stackrestore") == 0) {
            func->stackIgnored();
            return;
        }

        if (funcName.substr(0,4).compare("llvm") == 0) {
            if (isCFunc(getCFunc(funcName))) {
                funcName = getCFunc(funcName);
            } else {
                std::replace(funcName.begin(), funcName.end(), '.', '_');
            }
        }
    } else {
        llvm::PointerType* PT = llvm::cast<llvm::PointerType>(callInst->getCalledValue()->getType());
        llvm::FunctionType* FT = llvm::cast<llvm::FunctionType>(PT->getElementType());
        type = Type::getType(FT->getReturnType());
        isFuncPointer = true;

        if (llvm::InlineAsm* IA = llvm::dyn_cast<llvm::InlineAsm>(callInst->getCalledValue())) {
            std::string asmString = "\"" + IA->getAsmString() + "\"";
            if (!IA->getConstraintString().empty()) {
                asmString += ", \"" + IA->getConstraintString() + "\"";
            }

            func->callExprMap[value] = std::make_unique<AsmExpr>(asmString);
            func->createExpr(value, std::make_unique<Value>(func->getVarName(), type->clone()));
            func->callValueMap[value] = std::make_unique<EqualsExpr>(func->getExpr(value), func->callExprMap[value].get());

            if (!isConstExpr) {
                abstractSyntaxTree.push_back(func->getExpr(&ins));
                abstractSyntaxTree.push_back(func->callValueMap[&ins].get());
            }

            return;
        }

        funcName = func->getExpr(callInst->getCalledValue())->toString();
    }

    for (const llvm::Use& param : callInst->arg_operands()) {
        if (!func->getExpr(param)) {
            if (llvm::PointerType* PT = llvm::dyn_cast<llvm::PointerType>(param->getType())) {
                if (PT->getElementType()->isFunctionTy()) {
                    func->createExpr(param, std::make_unique<Value>(param->getName().str(), std::make_unique<VoidType>()));
                } else {
                    createConstantValue(param);
                }
            } else {
                createConstantValue(param);
            }
        }
        params.push_back(func->getExpr(param));
    }

    if (funcName.compare("va_start") == 0) {
        params.push_back(func->lastArg);
    }

    if (VoidType* VT = dynamic_cast<VoidType*>(type.get())) {
        func->createExpr(value, std::make_unique<CallExpr>(funcName, params, type->clone(), isFuncPointer));

        if (!isConstExpr) {
            abstractSyntaxTree.push_back(func->getExpr(&ins));
        }
    } else {
        func->callExprMap[value] = std::make_unique<CallExpr>(funcName, params, type->clone(), isFuncPointer);
        func->createExpr(value, std::make_unique<Value>(func->getVarName(), type->clone()));
        func->callValueMap[value] = std::make_unique<EqualsExpr>(func->getExpr(value), func->callExprMap[value].get());

        if (!isConstExpr) {
            abstractSyntaxTree.push_back(func->getExpr(&ins));
            abstractSyntaxTree.push_back(func->callValueMap[&ins].get());
        }
    }
}

void Block::parseCastInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val) {
    if (func->getExpr(ins.getOperand(0)) == nullptr) {
        createConstantValue(ins.getOperand(0));
    }
    Expr* expr = func->getExpr(ins.getOperand(0));

    const llvm::CastInst* CI = llvm::cast<const llvm::CastInst>(&ins);

    if (!isConstExpr) {
        func->createExpr(&ins, std::make_unique<CastExpr>(expr, Type::getType(CI->getDestTy())));
    } else {
        func->createExpr(val, std::make_unique<CastExpr>(expr, Type::getType(CI->getDestTy())));
    }
}

void Block::parseSelectInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val) {
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

    if (!val0 || !val1) {
        llvm::outs() << ins << "\n";
        llvm::outs().flush();
    }

    if (!isConstExpr) {
        func->createExpr(&ins, std::make_unique<SelectExpr>(cond, val0, val1));
    } else {
        func->createExpr(val, std::make_unique<SelectExpr>(cond, val0, val1));
    }
}

void Block::parseGepInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val) {
    const llvm::GetElementPtrInst* gepInst = llvm::cast<llvm::GetElementPtrInst>(&ins);

    if (!func->getExpr(gepInst->getOperand(0))) {
        createConstantValue(gepInst->getOperand(0));
    }
    Expr* expr = func->getExpr(gepInst->getOperand(0));
    auto gepExpr = std::make_unique<GepExpr>(expr, Type::getType(gepInst->getType()));

    std::string indexValue;
    llvm::Type* prevType = gepInst->getOperand(0)->getType();

    bool isStruct = false;
    int advance = 0;
    llvm::PointerType* PT = llvm::cast<llvm::PointerType>(gepInst->getOperand(0)->getType());
    if (PT->getElementType()->isStructTy()) {
        isStruct = true;
        llvm::StructType* ST = llvm::cast<llvm::StructType>(PT->getElementType());
        std::string structName = ST->getName().str();
        if (structName.substr(0, 6).compare("struct") == 0) {
            structName.erase(0, 7);
        } else {
            //union
            structName.erase(0, 6);
        }
        std::string varName = func->getExpr(gepInst->getOperand(0))->toString();

        /*if (varName.at(0) == '&') {
            varName = func->getExpr(gepInst->getOperand(0))->toString().erase(0,2);
            varName = varName.erase(varName.length() - 1, varName.length());
        }*/

        if (ins.getNumOperands() > 2) {
            advance = 2;
            structElements[&ins] = std::make_unique<StructElement>(func->getStruct(structName), expr, llvm::cast<llvm::ConstantInt>(gepInst->getOperand(2))->getSExtValue(), llvm::cast<llvm::ConstantInt>(gepInst->getOperand(1))->getSExtValue());
            refs[structElements[&ins].get()] = std::make_unique<RefExpr>(structElements[&ins].get());
            gepExpr = std::make_unique<GepExpr>(refs[structElements[&ins].get()].get(), Type::getType(gepInst->getType()));
        } else {
            advance = 1;
            if (auto CI = llvm::dyn_cast<llvm::ConstantInt>(gepInst->getOperand(1))) {
                indexValue = std::to_string(CI->getSExtValue());
            } else {
                indexValue = func->getExpr(gepInst->getOperand(1))->toString();
            }

            gepExpr->addArg(Type::getType(prevType), indexValue);
        }
    }

    for (auto it = llvm::gep_type_begin(gepInst); it != llvm::gep_type_end(gepInst); it++) {
        if (isStruct) {
            std::advance(it, advance);
            if (it == llvm::gep_type_end(gepInst)) {
                break;
            }
            isStruct = false;
        }

        /*if (auto CI = llvm::dyn_cast<llvm::ConstantInt>(it.getOperand())) {
            indexValue = std::to_string(CI->getSExtValue());
        } else {*/
        if (!func->getExpr(it.getOperand())) {
            createConstantValue(it.getOperand());
        }
        indexValue = func->getExpr(it.getOperand())->toString();
        //}


        if (prevType->isArrayTy()) {
            gepExpr->addArg(Type::getType(prevType), indexValue);
        } else {
            gepExpr->addArg(std::make_unique<PointerType>(Type::getType(prevType)), indexValue);
        }
        prevType = it.getIndexedType();
    }

    if (!isConstExpr) {
        func->createExpr(&ins, std::move(gepExpr));
    } else {
        func->createExpr(val, std::move(gepExpr));
    }
}

void Block::parseLLVMInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val) {
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
        parseBinaryInstruction(ins, isConstExpr, val);
        break;
    case llvm::Instruction::Alloca:
        parseAllocaInstruction(ins, isConstExpr, val);
        break;
    case llvm::Instruction::Load:
        parseLoadInstruction(ins, isConstExpr, val);
        break;
    case llvm::Instruction::Store:
        parseStoreInstruction(ins, isConstExpr, val);
        break;
    case llvm::Instruction::ICmp:
    case llvm::Instruction::FCmp:
        parseCmpInstruction(ins, isConstExpr, val);
        break;
    case llvm::Instruction::Br:
        parseBrInstruction(ins, isConstExpr, val);
        break;
    case llvm::Instruction::Ret:
        parseRetInstruction(ins, isConstExpr, val);
        break;
    case llvm::Instruction::Switch:
        parseSwitchInstruction(ins, isConstExpr, val);
        break;
    case llvm::Instruction::Unreachable:
    case llvm::Instruction::Fence:
        parseAsmInst(ins, isConstExpr, val);
        break;
    case llvm::Instruction::Shl:
    case llvm::Instruction::LShr:
    case llvm::Instruction::AShr:
        parseShiftInstruction(ins, isConstExpr, val);
        break;
    case llvm::Instruction::Call:
        parseCallInstruction(ins, isConstExpr, val);
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
        parseCastInstruction(ins, isConstExpr, val);
        break;
    case llvm::Instruction::Select:
        parseSelectInstruction(ins, isConstExpr, val);
        break;
    case llvm::Instruction::GetElementPtr:
        parseGepInstruction(ins, isConstExpr, val);
        break;
    default:
        llvm::outs() << "File contains unsupported instruction!\n";
        llvm::outs() << ins << "\n";
        throw std::invalid_argument("");
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

        if (llvm::DIDerivedType* dtype = llvm::dyn_cast<llvm::DIDerivedType>(localVar->getType())) {
            if (dtype->getTag() == llvm::dwarf::DW_TAG_const_type) {
                variable->getType()->isConst = true;
            }

            if (isVoidType(dtype)) {
                llvm::PointerType* PT = llvm::cast<llvm::PointerType>(referredVal->getType());
                variable->setType(Type::getType(PT->getElementType(), true));
            }
        }

        if (llvm::DICompositeType* ctype = llvm::dyn_cast<llvm::DICompositeType>(localVar->getType())) {
            if (isVoidType(ctype)) {
                llvm::PointerType* PT = llvm::cast<llvm::PointerType>(referredVal->getType());
                variable->setType(Type::getType(PT->getElementType(), true));
            }
        }

        std::regex varName("var[0-9]+");
        std::regex constGlobalVarName("ConstGlobalVar_.+");
        if (!std::regex_match(localVar->getName().str(), varName) && !std::regex_match(localVar->getName().str(), constGlobalVarName)) {
            variable->valueName = localVar->getName();
        }

        if (type && type->getName().str().compare(0, 8, "unsigned") == 0) {
            if (IntegerType* IT = dynamic_cast<IntegerType*>(variable->getType())) {
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

void Block::createConstantValue(const llvm::Value* val) {
    if (auto CPN = llvm::dyn_cast<const llvm::ConstantPointerNull>(val)) {
        func->createExpr(val, std::make_unique<Value>("0", std::make_unique<VoidType>()));
    }
    if (auto CI = llvm::dyn_cast<const llvm::ConstantInt>(val)) {
        func->createExpr(val, std::make_unique<Value>(std::to_string(CI->getSExtValue()), std::make_unique<IntType>(false)));
    }
    if (auto CFP = llvm::dyn_cast<const llvm::ConstantFP>(val)) {
        func->createExpr(val, std::make_unique<Value>(std::to_string(CFP->getValueAPF().convertToDouble()), std::make_unique<FloatType>()));
    }
    if (auto CE = llvm::dyn_cast<const llvm::ConstantExpr>(val)) {
        parseLLVMInstruction(*CE->getAsInstruction(), true, val);
    }
}

bool Block::isCFunc(const std::string& func) {
    return C_FUNCTIONS.find(func) != C_FUNCTIONS.end();
}

std::string Block::getCFunc(const std::string& func) {
    std::regex function("llvm\\.(\\w+)(\\..+){0,1}");
    std::smatch match;
    if (std::regex_search(func, match, function)) {
        return match[1].str();
    }

    return "";
}

bool Block::isVoidType(llvm::DITypeRef type) {
    if (llvm::DIDerivedType* dtype = llvm::dyn_cast<llvm::DIDerivedType>(type)) {
        if (!dtype->getBaseType()) {
            return true;
        }

        return isVoidType(dtype->getBaseType());
    }

    if (llvm::DICompositeType* ctype = llvm::dyn_cast<llvm::DICompositeType>(type)) {
        if (!ctype->getBaseType()) {
            return true;
        }

        return isVoidType(ctype->getBaseType());
    }

    return false;
}
