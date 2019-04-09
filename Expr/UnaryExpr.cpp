#include "UnaryExpr.h"

#include "llvm/Support/raw_ostream.h"

/*
 * UnaryExpr classes
 */

UnaryExpr::UnaryExpr(Expr *expr) {
    this->expr = expr;
    if (expr) {
        setType(expr->getType()->clone());
    }
}

RefExpr::RefExpr(Expr* expr) :
    UnaryExpr(expr) {
    setType(std::make_unique<PointerType>(expr->getType()->clone()));
}

void RefExpr::print() const {
    llvm::outs() << toString();
}

std::string RefExpr::toString() const {
    return "&(" + expr->toString() + ")";
}

GepExpr::GepExpr(Expr* expr, std::unique_ptr<Type> type)
    : UnaryExpr(expr) {
    setType(std::move(type));
}

void GepExpr::print() const {
    llvm::outs() << this->toString();
}

std::string GepExpr::toString() const {
    std::string print = expr->toString();
    bool move;
    bool se = false;

    for (unsigned i = 0; i < args.size(); i++) {
        se = false;
        move = args[i].second->toString().compare("0") != 0;

        if (i == 0) {
            if (move) {
                if (auto SE = dynamic_cast<StructElement*>(args[i].second)) {
                    print.append(SE->toString());
                } else {
                    print.append(" + " + args[i].second->toString());
                }
            }
            continue;
        }

        if (auto AT = dynamic_cast<ArrayType*>(args[i].first.get())) {
            if (AT->isPointerArray && AT->pointer->isFuncPointer) {
                print = "(((" + args[i].first->toString() + "(**)" + AT->pointer->params + ")" + print;
            } else {
                print = "(*((" + args[i].first->toString() + "(*)" + AT->sizeToString() + ")" + print;
            }
        } else if (auto PT = dynamic_cast<PointerType*>(args[i].first.get())) {

        } else {
            if (auto SE = dynamic_cast<StructElement*>(args[i].second)) {
                print += SE->toString();
                move = false;
                se = true;
            } else {
                print = "(((" + args[i].first->toString() + ")" + print;
            }
        }

        if (move) {
            if (auto SE = dynamic_cast<StructElement*>(args[i].second)) {
                print += "))";
                print.append(args[i].second->toString());
            } else {
                print.append(") + " + args[i].second->toString() + ")");
            }
        } else if (!se) {
            print.append("))");
        }

        if (auto AT = dynamic_cast<ArrayType*>(args[i].first.get())) {
            if (i != args.size() - 1) {
                print = "(*(" + print + "))";
            }
        }
    }

    return print;
}

void GepExpr::addArg(std::unique_ptr<Type> type, Expr* index) {
    args.push_back(std::make_pair(std::move(type), index));
}

DerefExpr::DerefExpr(Expr* expr) :
    UnaryExpr(expr) {
    if (auto PT = dynamic_cast<PointerType*>(expr->getType())) {
        setType(PT->type->clone());
    } else if (auto TD = dynamic_cast<TypeDef*>(expr->getType())) {
        setType(TD->getDerefType());
    }
}

void DerefExpr::print() const {
    llvm::outs() << toString();
}

std::string DerefExpr::toString() const {
    if (auto refExpr = dynamic_cast<RefExpr*>(expr)) {
        return refExpr->expr->toString();
    }

    return "*(" + expr->toString() + ")";
}

RetExpr::RetExpr(Expr* ret)
    : UnaryExpr(ret) { }

RetExpr::RetExpr()
    : UnaryExpr(nullptr) { }

void RetExpr::print() const {
    llvm::outs() << toString();
}

std::string RetExpr::toString() const {
    std::string ret;

    ret += "return";
    if (expr) {
        ret += " " + expr->toString();
    }

    return ret + ";";
}

CastExpr::CastExpr(Expr* expr, std::unique_ptr<Type> type)
    : UnaryExpr(expr) {
    setType(std::move(type));
}

void CastExpr::print() const {
    llvm::outs() << toString();
}

std::string CastExpr::toString() const {
    std::string ret = "(" + getType()->toString();
    if (auto PT = dynamic_cast<PointerType*>(getType())) {
        if (PT->isFuncPointer || PT->isArrayPointer) {
            ret += " (";
            for (unsigned i = 0; i < PT->levels; i++) {
                ret += "*";
            }
            ret += ")";
        }
        if (PT->isArrayPointer) {
            ret = ret + PT->sizes;
        }
        if (PT->isFuncPointer) {
            ret += PT->params;
        }
    }
    return ret + ")" + "(" + expr->toString() + ")";
}
