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

DerefExpr::DerefExpr(Expr* expr) :
    UnaryExpr(expr) {
    if (auto PT = dynamic_cast<PointerType*>(expr->getType())) {
        setType(PT->type->clone());
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
        if (PT->isArrayPointer) {
            ret += " (";
            for (unsigned i = 0; i < PT->levels; i++) {
                ret += "*";
            }
            ret += ")" + PT->sizes;
        }
    }
    return ret + ")" + "(" + expr->toString() + ")";
}
