#include "Expr.h"
#include "UnaryExpr.h"

#include "llvm/Support/raw_ostream.h"

Struct::Struct(const std::string& name)
    : name(name),
      isPrinted(false) {
    setType(std::make_unique<StructType>(this->name));
}

void Struct::print() const {
    llvm::outs() << toString();
}

std::string Struct::toString() const {
    std::string ret;

    ret += "struct ";
    ret += name + " {\n";

    for (const auto& item : items) {
        std::string faPointer;

        ret += "    " + item.first->toString();

        if (auto PT = dynamic_cast<PointerType*>(item.first.get())) {
            if (PT->isArrayPointer) {
                faPointer = " (";
                for (unsigned i = 0; i < PT->levels; i++) {
                    faPointer += "*";
                }
                faPointer += item.second + ")" + PT->sizes;
            }
        }

        if (faPointer.empty()) {
            ret += " ";

            if (auto AT = dynamic_cast<ArrayType*>(item.first.get())) {
                if (AT->isPointerArray && AT->pointer->isArrayPointer) {
                    ret += "(";
                    for (unsigned i = 0; i < AT->pointer->levels; i++) {
                        ret += "*";
                    }
                    ret += item.second + AT->sizeToString() + ")" + AT->pointer->sizes;
                } else {
                    ret += item.second + AT->sizeToString();
                }
            } else {
                ret += item.second;
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

StructElement::StructElement(Struct* strct, Expr* expr, unsigned element)
    : strct(strct),
      expr(expr),
      element(element) {
    setType(strct->items[element].first->clone());
}

void StructElement::print() const {
    llvm::outs() << toString();
}

std::string StructElement::toString() const {
    std::string ret = "(";
    if (auto PT = dynamic_cast<PointerType*>(expr->getType())) {
        return "(" + expr->toString() + ")->" + strct->items[element].second;
    }

    return "(" + expr->toString() + ")." + strct->items[element].second;
}

ArrayElement::ArrayElement(Expr* expr, Expr* elem)
    : expr(expr),
      element(elem) {
    ArrayType* AT = static_cast<ArrayType*>(expr->getType());
    setType(AT->type->clone());
}

ArrayElement::ArrayElement(Expr* expr, Expr* elem, std::unique_ptr<Type> type)
    : expr(expr),
      element(elem) {
    setType(std::move(type));
}

void ArrayElement::print() const {
    llvm::outs() << toString();
}

std::string ArrayElement::toString() const {
    return "(" + expr->toString() + ")[" + element->toString() + "]";
}

ExtractValueExpr::ExtractValueExpr(std::vector<std::unique_ptr<Expr>>& indices) {
    for (auto& idx : indices) {
        this->indices.push_back(std::move(idx));
    }

    setType(this->indices[this->indices.size() - 1]->getType()->clone());
}

void ExtractValueExpr::print() const {
    llvm::outs() << toString();
}

std::string ExtractValueExpr::toString() const {
    return indices[indices.size() - 1]->toString();
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
    if (valueName.compare("0") == 0) {
        return valueName;
    }

    if (!init) {
        std::string ret;
        if (auto PT = dynamic_cast<const PointerType*>(getType())) {
            if (PT->isArrayPointer && valueName.compare("0") != 0) {
                ret = "(";
                for (unsigned i = 0; i < PT->levels; i++) {
                    ret += "*";
                }
                ret += valueName + ")";
            }

            if (PT->isArrayPointer) {
                ret = ret + PT->sizes;
            }

            if (!ret.empty()) {
                return ret;
            }
        }

        if (auto AT = dynamic_cast<const ArrayType*>(getType())) {
            if (AT->isPointerArray && AT->pointer->isArrayPointer) {
                ret = "(";
                for (unsigned i = 0; i < AT->pointer->levels; i++) {
                    ret += "*";
                }
                return ret + valueName + AT->sizeToString() + ")" + AT->pointer->sizes;
            } else {
                return valueName + AT->sizeToString();
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
        std::string ret = getType()->toString() + " ";
        if (auto AT = dynamic_cast<const ArrayType*>(getType())) {
            if (AT->isPointerArray && AT->pointer->isArrayPointer) {
                ret += "(";
                for (unsigned i = 0; i < AT->pointer->levels; i++) {
                    ret += "*";
                }
                ret += valueName + AT->sizeToString() + ")" + AT->pointer->sizes;
            } else {
                ret += " " + valueName + AT->sizeToString();;
            }
        } else if (auto PT = dynamic_cast<const PointerType*>(getType())) {
            if (PT->isArrayPointer && valueName.compare("0") != 0) {
                ret += "(";
                for (unsigned i = 0; i < PT->levels; i++) {
                    ret += "*";
                }
                ret += valueName + ")";
            } else {
                ret += " " + valueName;

                if (!value.empty()) {
                    ret += " = " + value;
                }

                return ret + ";";
            }

            if (PT->isArrayPointer) {
                ret = ret + PT->sizes;
            }
        } else {
            ret += valueName;
        }

        if (!value.empty()) {
            ret += " = " + value;
        }

        return ret + ";";
    }

    return valueName;
}

std::string GlobalValue::declToString() const {
    std::string ret = getType()->toString();
    if (auto AT = dynamic_cast<const ArrayType*>(getType())) {
        if (AT->isPointerArray && AT->pointer->isArrayPointer) {
            ret += " (";
            for (unsigned i = 0; i < AT->pointer->levels; i++) {
                ret += "*";
            }
            ret += valueName + AT->sizeToString() + ")" + AT->pointer->sizes;
        } else {
            ret += " " + valueName + AT->sizeToString();;
        }
    } else if (auto PT = dynamic_cast<const PointerType*>(getType())) {
        if (PT->isArrayPointer && valueName.compare("0") != 0) {
            ret += "(";
            for (unsigned i = 0; i < PT->levels; i++) {
                ret += "*";
            }
            ret += valueName + ")";
        } else {
            return ret + " " + valueName + ";";
        }

        if (PT->isArrayPointer) {
            ret = ret + PT->sizes;
        }


    } else {
        ret += " " + valueName;
    }

    return ret + ";";
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

    if (!def.empty()) {
        ret += "    default:\n        goto ";
        ret += def;
        ret += ";\n";
    }

    return ret + "    }";
}

AsmExpr::AsmExpr(const std::string& inst, const std::vector<std::pair<std::string, Expr*>>& output, const std::vector<std::pair<std::string, Expr*>>& input, const std::string& clobbers)
    : inst(inst),
      output(output),
      input(input),
      clobbers(clobbers) {}

void AsmExpr::print() const {
    llvm::outs() << toString();
}

std::string AsmExpr::toString() const {
    std::string ret = "__asm__(\"" + inst + "\"\n        : ";
    if (!output.empty()) {
        bool first = true;
        for (const auto& out : output) {
            if (!out.second) {
                break;
            }
            if (!first) {
                ret += ", ";
            }
            first = false;

            ret += out.first + " (" + out.second->toString() + ")";
        }
    }

    ret += "\n        : ";

    if (!input.empty()) {
        bool first = true;
        for (const auto& in : input) {
            if (!first) {
                ret += ", ";
            }
            first = false;

            ret += in.first + " (" + in.second->toString() + ")";
        }
    }

    ret += "\n        : ";

    if (!clobbers.empty()) {
        ret += clobbers;
    }

    return ret + "\n    );";
}

void AsmExpr::addOutputExpr(Expr* expr, unsigned pos) {
    for (unsigned i = pos; i < output.size(); i++) {
        if (!output[i].second) {
            output[i].second = expr;
            break;
        }
    }
}

CallExpr::CallExpr(Expr* funcValue, const std::string &funcName, std::vector<Expr*> params, std::unique_ptr<Type> type)
    : funcName(funcName),
      params(params),
      funcValue(funcValue) {
    setType(std::move(type));
}

void CallExpr::print() const {
    llvm::outs() << toString();
}

std::string CallExpr::toString() const {
    std::string ret;

    if (funcValue) {
        ret += "(" + funcValue->toString() + ")(";
    } else {
        ret += funcName + "(";
    }

    ret += paramsToString();

    if (const auto VT = dynamic_cast<const VoidType*>(getType())) {
        return ret + ");";
    }
    return ret + ")";
}

void CallExpr::printParams() const {
    llvm::outs() << paramsToString();
}

std::string CallExpr::paramsToString() const {
    std::string ret;

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

    return ret;
}

PointerMove::PointerMove(std::unique_ptr<Type> ptrType, Expr* pointer, Expr* move)
    : ptrType(std::move(ptrType)),
      pointer(pointer),
      move(move) {
    if (auto PT = dynamic_cast<PointerType*>(this->ptrType.get())) {
        setType(PT->type->clone());
    }
}

void PointerMove::print() const {
    llvm::outs() << toString();
}

std::string PointerMove::toString() const {
    std::string ret;

    if (move->toString().compare("0") == 0) {
        return pointer->toString();
    }

    ret += "*(((" + ptrType->toString();

    auto PT = static_cast<PointerType*>(ptrType.get());

    if (PT->isArrayPointer) {
        ret += "(";
        for (unsigned i = 0; i < PT->levels; i++) {
            ret += "*";
        }
        ret += ")" + PT->sizes;
    }

    return ret + ")(" + pointer->toString() + ")) + (" + move->toString() + "))";
}

GepExpr::GepExpr(std::vector<std::unique_ptr<Expr>>& indices) {
    for (auto& index : indices) {
        this->indices.push_back(std::move(index));
    }

    setType(this->indices[this->indices.size() - 1]->getType()->clone());
}

void GepExpr::print() const {
    llvm::outs() << toString();
}

std::string GepExpr::toString() const {
    return indices[indices.size() - 1]->toString();
}

SelectExpr::SelectExpr(Expr* comp, Expr* l, Expr* r) :
    left(l),
    right(r),
    comp(comp) {
    setType(l->getType()->clone());
}

void SelectExpr::print() const {
    llvm::outs() << toString();
}

std::string SelectExpr::toString() const {
    return "(" + comp->toString() + ") ? " + left->toString() + " : " + right->toString();
}

