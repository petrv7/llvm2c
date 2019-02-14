#include "Expr.h"

#include "llvm/Support/raw_ostream.h"

GepExpr::GepExpr(Expr* element)
    : element(element) { }

void GepExpr::print() const {
    llvm::outs() << this->toString();
}

std::string GepExpr::toString() const {
    std::string print = "(" + element->toString();
    print.append(" + " + args[0].second + ")");

    for (unsigned int i = 1; i < args.size(); i++) {
        print = "((" + args[i].first->toString() + ")" + print;
        print.append(" + " + args[i].second + ")");
    }

    print = "*(" + print + ")";
    return print;
}

void GepExpr::addArg(std::unique_ptr<Type> type, const std::string& index) {
    args.push_back(std::make_pair(std::move(type), index));
}

Struct::Struct(const std::string & name)
    : name(name) { }

void Struct::print() const {
    llvm::outs() << "struct " << name << " {\n";
    for (const auto& item : items) {
        llvm::outs() << "    ";
        item.first->print();
        llvm::outs() << " " << item.second;

        if (auto AT = dynamic_cast<ArrayType*>(item.first.get())) {
            AT->printSize();
        }

        llvm::outs() << ";\n";
    }
    llvm::outs() << "};\n";
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
    ret += "};\n";

    return ret;
}

void Struct::addItem(std::unique_ptr<Type> type, const std::string& name) {
    items.push_back(std::make_pair(std::move(type), name));
}

Value::Value(const std::string& s, std::unique_ptr<Type> type) {
    this->type = std::move(type);
    val = s;
    typePrinted = false;
}

void Value::print() const {
    llvm::outs() << val;
}

std::string Value::toString() const {
    return val;
}

JumpExpr::JumpExpr(const std::string &block)
    : block(block) { }

void JumpExpr::print() const {
    llvm::outs() << "goto ";
    llvm::outs() << block;
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
    if (cmp != nullptr) {
        llvm::outs() << "if (";
        llvm::outs() << cmp->toString();
        llvm::outs() << ") {\n    goto ";
        llvm::outs() << trueBlock;
        llvm::outs() << ";\n}";
        llvm::outs() << " else {\n    goto ";
        llvm::outs() << falseBlock;
        llvm::outs() << ";\n}";
    } else {
        llvm::outs() << "goto ";
        llvm::outs() << trueBlock;
        llvm::outs() << ";";
    }
}

std::string IfExpr::toString() const {
    if (cmp != nullptr) {
        return "if (" + cmp->toString() + ") {\n    goto " + trueBlock + ";\n} else {\n    goto " + falseBlock + ";\n}";
    }

    return "goto " + trueBlock + ";";
}

SwitchExpr::SwitchExpr(Expr* cmp, const std::string &def, std::map<int, std::string> cases)
    : cmp(cmp),
      def(def),
      cases(cases) { }

void SwitchExpr::print() const {
    llvm::outs() << "switch(";
    cmp->print();
    llvm::outs() << ") {\n";

    for (const auto &iter : cases) {
        llvm::outs() << "case " << iter.first << ":\n";
        llvm::outs() << "    goto " << iter.second << ";\n";
    }

    llvm::outs() << "default:\n";
    llvm::outs() << "    goto " << def << ";\n";
    llvm::outs() << "}";
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
    llvm::outs() << "asm(\"" << inst << "\");";
}

std::string AsmExpr::toString() const {
    return "asm(\"" + inst + "\");";
}

CallExpr::CallExpr(const std::string &funcName, std::vector<Expr*> params)
    : funcName(funcName),
      params(params) { }

void CallExpr::print() const {
    llvm::outs() << funcName << "(";

    bool first = true;
    for (auto param : params) {
        if (first) {
            param->print();
        } else {
            llvm::outs() << ", ";
            param->print();
        }
        first = false;
    }

    llvm::outs() << ");";
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

    ret += ");";

    return ret;
}
