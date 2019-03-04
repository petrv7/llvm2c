#include "Expr.h"
#include "UnaryExpr.h"

#include "llvm/Support/raw_ostream.h"

Struct::Struct(const std::string & name)
    : name(name) {}

void Struct::print() const {
    llvm::outs() << toString();
}

std::string Struct::toString() const {
    std::string ret;

    ret += "struct " + name + " {\n";
    for (const auto& item : items) {
        ret += "    ";
        ret += item.first->toString();
        ret += " " + item.second;

        if (auto AT = dynamic_cast<ArrayType*>(item.first.get())) {
            ret += AT->sizeToString();
        }

        ret += ";\n";
    }
    ret += "};";

    return ret;
}

void Struct::addItem(std::unique_ptr<Type> type, const std::string& name) {
    items.push_back(std::make_pair(std::move(type), name));
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
    if (auto PT = dynamic_cast<PointerType*>(type.get())) {
        if (PT->isFuncPointer) {
            if (!init) {
                std::string ret = "(";
                for (unsigned i = 0; i < PT->levels; i++) {
                    ret += "*";
                }
                return ret + valueName + ")";
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
        std::string ret = type->toString() + " " + valueName.substr(1, valueName.length());
        if (ArrayType* AT = dynamic_cast<ArrayType*>(type.get())) {
            ret += AT->sizeToString();
        }
        if (!value.empty()) {
            ret += " = " + value;
        }

        return ret + ";";
    }

    return valueName;
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
    if (cmp != nullptr) {
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

    ret += "switch(";
    ret += cmp->toString();
    ret += ") {\n";

    for (const auto &iter : cases) {
        ret += "   case ";
        ret += iter.first;
        ret += ":\n";
        ret += "        goto ";
        ret += iter.second;
        ret += ";\n";
    }

    ret += "}";

    return ret;
}

AsmExpr::AsmExpr(const std::string &inst)
    :inst(inst) {}

void AsmExpr::print() const {
    llvm::outs() << toString();
}

std::string AsmExpr::toString() const {
    return "asm(\"" + inst + "\");";
}

CallExpr::CallExpr(const std::string &funcName, std::vector<Expr*> params, std::unique_ptr<Type> type)
    : funcName(funcName),
      params(params),
      isUsed(false) {
    setType(std::move(type));
}

void CallExpr::print() const {
    llvm::outs() << toString();
}

std::string CallExpr::toString() const {
    std::string ret;

    ret += funcName + "(";

    bool first = true;
    for (auto param : params) {
        if (first) {
            ret += param->toString();
        } else {
            ret += ", ";
            ret += param->toString();
        }
        first = false;
    }

    ret += ")";

    return ret;
}
