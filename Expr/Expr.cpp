#include "Expr.h"
#include "UnaryExpr.h"

#include "llvm/Support/raw_ostream.h"

Struct::Struct(const std::string & name)
    : name(name),
      isPrinted(false) {
    setType(std::make_unique<StructType>(this->name));
}

void Struct::print() const {
    llvm::outs() << toString();
}

std::string Struct::toString() const {
    if (name.compare("va_list") == 0) {
        return "";
    }

    std::string ret;

    ret += "struct " + name + " {\n";
    for (const auto& item : items) {
        std::string faPointer;

        ret += "    ";
        ret += item.first->toString();

        if (auto PT = dynamic_cast<PointerType*>(item.first.get())) {
            if (PT->isFuncPointer || PT->isArrayPointer) {
                faPointer = " (";
                for (unsigned i = 0; i < PT->levels; i++) {
                    faPointer += "*";
                }
                faPointer += item.second + ")";
            }
            if (PT->isArrayPointer) {
                faPointer = faPointer + "[" + std::to_string(PT->size) + "]";
            }
            if (PT->isFuncPointer) {
                faPointer += PT->params;
            }
        }

        if (faPointer.empty()) {
            ret += " " + item.second;

            if (auto AT = dynamic_cast<ArrayType*>(item.first.get())) {
                ret += AT->sizeToString();
            }
        } else {
            ret += faPointer;
        }

        ret += ";\n";
    }
    ret += "};";

    return ret;
}

void Struct::addItem(std::unique_ptr<Type> type, const std::string& name) {
    items.push_back(std::make_pair(std::move(type), name));
}

StructElement::StructElement(Struct* strct, Expr* expr, long element, unsigned int move)
    : strct(strct),
      expr(expr),
      element(element),
      move(move) {
    setType(strct->items[element].first->clone());
}

void StructElement::print() const {
    llvm::outs() << toString();
}

std::string StructElement::toString() const {
    std::string ret = "(";
    if (auto PT = dynamic_cast<PointerType*>(expr->getType())) {
        ret += expr->toString();
        //return "(" + expr->toString() + " + " + std::to_string(move) + ")->" + strct->items[element].second;
    } else {
        ret += "&" + expr->toString();
    }

    if (move == 0) {
        return ret + ")->" + strct->items[element].second;
    } else {
        return ret + " + " + std::to_string(move) + ")->" + strct->items[element].second;
    }
    //return "(&" + expr->toString() + " + " + std::to_string(move) + ")->" + strct->items[element].second;
}

Value::Value(const std::string& valueName, std::unique_ptr<Type> type) {
    setType(std::move(type));
    this->valueName = valueName;
    init = false;
}

void Value::print() const {
    llvm::outs() << toString();
}

std::string Value::toString() const {
    if (!init) {
        if (auto PT = dynamic_cast<PointerType*>(getType())) {
            std::string ret;
            if (PT->isFuncPointer || PT->isArrayPointer) {
                ret = "(";
                for (unsigned i = 0; i < PT->levels; i++) {
                    ret += "*";
                }
                ret += valueName + ")";
            }
            if (PT->isArrayPointer) {
                ret = ret + "[" + std::to_string(PT->size) + "]";
            }

            if (!ret.empty()) {
                return ret;
            }
        }
    }

    return valueName;
}

GlobalValue::GlobalValue(const std::string& varName, const std::string& value, std::unique_ptr<Type> type)
    : Value(varName, std::move(type)),
      value(value) { }

void GlobalValue::print() const {
    llvm::outs() << toString();
}

std::string GlobalValue::toString() const {
    if (!init) {
        std::string ret = getType()->toString() + " " + valueName;
        if (ArrayType* AT = dynamic_cast<ArrayType*>(getType())) {
            ret += AT->sizeToString();
        }
        if (!value.empty()) {
            ret += " = " + value;
        }

        return ret + ";";
    }

    return valueName;
}

std::string GlobalValue::declToString() const {
    std::string ret = getType()->toString() + " " + valueName;
    if (ArrayType* AT = dynamic_cast<ArrayType*>(getType())) {
        ret += AT->sizeToString();
    }

    return ret + ";";
}

JumpExpr::JumpExpr(const std::string &block)
    : block(block) {}

void JumpExpr::print() const {
    llvm::outs() << toString();
}

std::string JumpExpr::toString() const {
    return "goto " + block;
}

IfExpr::IfExpr(Expr* cmp, const std::string& trueBlock, const std::string& falseBlock)
    : cmp(cmp),
      trueBlock(trueBlock),
      falseBlock(falseBlock) {}

IfExpr::IfExpr(const std::string &trueBlock)
    : cmp(nullptr),
      trueBlock(trueBlock),
      falseBlock("") {}

void IfExpr::print() const {
    llvm::outs() << toString();
}

std::string IfExpr::toString() const {
    if (cmp) {
        return "if (" + cmp->toString() + ") {\n        goto " + trueBlock + ";\n    } else {\n        goto " + falseBlock + ";\n    }";
    }

    return "goto " + trueBlock + ";";
}

SwitchExpr::SwitchExpr(Expr* cmp, const std::string &def, std::map<int, std::string> cases)
    : cmp(cmp),
      def(def),
      cases(cases) {}

void SwitchExpr::print() const {
    llvm::outs() << toString();
}

std::string SwitchExpr::toString() const {
    std::string ret;

    ret += "switch(" + cmp->toString();
    ret += ") {\n";

    for (const auto &iter : cases) {
        ret += "    case " + std::to_string(iter.first);
        ret += ":\n        goto " + iter.second;
        ret += ";\n";
    }

    return ret + "}";
}

AsmExpr::AsmExpr(const std::string &inst)
    :inst(inst) {}

void AsmExpr::print() const {
    llvm::outs() << toString();
}

std::string AsmExpr::toString() const {
    return "__asm__(\"" + inst + "\");";
}

CallExpr::CallExpr(const std::string &funcName, std::vector<Expr*> params, std::unique_ptr<Type> type, bool isFuncPointer)
    : funcName(funcName),
      params(params),
      isFuncPointer(isFuncPointer)
{
    setType(std::move(type));
}

void CallExpr::print() const {
    llvm::outs() << toString();
}

std::string CallExpr::toString() const {
    std::string ret;

    if (isFuncPointer) {
        ret += "(" + funcName + ")(";
    } else {
        ret += funcName + "(";
    }
    if (funcName.compare("va_start") == 0 || funcName.compare("va_end") == 0) {
        ret += "(void*)(";
    }

    bool first = true;
    for (auto param : params) {
        if (first) {
            ret += param->toString();
            if (funcName.compare("va_start") == 0 || funcName.compare("va_end") == 0) {
                ret += ")";
            }
        } else {
            ret += ", " + param->toString();
        }
        first = false;
    }

    if (auto VT = dynamic_cast<VoidType*>(getType())) {
        return ret + ");";
    }
    return ret + ")";
}
