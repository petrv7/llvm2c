#include "Expr.h"
#include "UnaryExpr.h"

#include "llvm/Support/raw_ostream.h"

GepExpr::GepExpr(Expr* element, std::unique_ptr<Type> type)
    : element(element) {
    setType(std::move(type));
}

void GepExpr::print() const {
    llvm::outs() << this->toString();
}

std::string GepExpr::toString() const {
    std::string print;
    unsigned int i = 1;

    if (UnaryExpr* UE = dynamic_cast<UnaryExpr*>(element)) {
        if (Value* val = dynamic_cast<Value*>(UE->expr)) {
            if (StructType* ST = dynamic_cast<StructType*>(val->type.get())) {
                print = args[0].second;
                i = 2;
            } else {
                print = element->toString();
            }
        } else {
            if (args[0].second == "0") {
                print = element->toString();
            } else {
                if (auto VT = dynamic_cast<VoidType*>(args[0].first.get())) {
                    print = args[0].second;
                } else {
                    print = "(" + element->toString();
                    print.append(" + " + args[0].second + ")");
                }
            }
        }
    } else if (GlobalValue* GV = dynamic_cast<GlobalValue*>(element)) {
        print = GV->toString();
    }

    for (i; i < args.size(); i++) {
        if (auto AT = dynamic_cast<ArrayType*>(args[i].first.get())) {
            if (auto ST = dynamic_cast<StructType*>(AT->type.get())) {
                print = "*(((" + args[i].first->toString() + AT->sizeToString() + ")" + print;
            } else {
                print = "(((" + args[i].first->toString() + AT->sizeToString() + ")" + print;
            }
        } else {
            print = "(((" + args[i].first->toString() + ")" + print;
        }
        print.append(") + " + args[i].second + ")");
    }

    return print;
}

void GepExpr::addArg(std::unique_ptr<Type> type, const std::string& index) {
    args.push_back(std::make_pair(std::move(type), index));
}

Struct::Struct(const std::string & name)
    : name(name) { }

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
    : block(block) { }

void JumpExpr::print() const {
    llvm::outs() << toString();
}

std::string JumpExpr::toString() const {
    return "goto " + block;
}

IfExpr::IfExpr(Expr* cmp, const std::string& trueBlock, const std::string& falseBlock)
    : cmp(cmp),
      trueBlock(trueBlock),
      falseBlock(falseBlock) { }

IfExpr::IfExpr(const std::string &trueBlock)
    : cmp(nullptr),
      trueBlock(trueBlock),
      falseBlock("") { }

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
      cases(cases) { }

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
    :inst(inst) { }

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
