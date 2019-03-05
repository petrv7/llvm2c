#include "UnaryExpr.h"

#include "llvm/Support/raw_ostream.h"

/*
 * UnaryExpr classes
 */

UnaryExpr::UnaryExpr(Expr *expr) {
    this->expr = expr;
    setType(expr->getType()->clone());
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
    unsigned int i = 0;
/*
    if (UnaryExpr* UE = dynamic_cast<UnaryExpr*>(expr)) {
        if (Value* val = dynamic_cast<Value*>(UE->expr)) {
            if (StructType* ST = dynamic_cast<StructType*>(val->type.get())) {
                print = args[0].second;
                i = 2;
            } else {
                print = expr->toString();
            }
        } else {
            if (args[0].second == "0") {
                print = expr->toString();
            } else {
                if (auto VT = dynamic_cast<VoidType*>(args[0].first.get())) {
                    print = args[0].second;
                } else {
                    print = "(" + expr->toString();
                    print.append(" + " + args[0].second + ")");
                }
            }
        }
    } else if (GlobalValue* GV = dynamic_cast<GlobalValue*>(expr)) {
        print = GV->toString();
    }
*/
    for (i; i < args.size(); i++) {
        if (i == 0) {
            print.append(" + " + args[i].second);
            continue;
        }
        if (auto AT = dynamic_cast<ArrayType*>(args[i].first.get())) {
            if (auto ST = dynamic_cast<StructType*>(AT->type.get())) {
                print = "*(*((" + args[i].first->toString() + "(*)" + AT->sizeToString() + ")" + print;
            } else {
                print = "(*((" + args[i].first->toString() + "(*)" + AT->sizeToString() + ")" + print;
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

DerefExpr::DerefExpr(Expr* expr) :
    UnaryExpr(expr) {
    auto PT = static_cast<PointerType*>(expr->getType());
    setType(PT->type->clone());
}

void DerefExpr::print() const {
    llvm::outs() << toString();
}

std::string DerefExpr::toString() const {
    if (auto refExpr = dynamic_cast<RefExpr*>(expr)) {
        return refExpr->expr->toString();
    } else {
        return "*(" + expr->toString() + ")";
    }
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
    if (expr != nullptr) {
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
    std::string ret;

    ret += "(" + getType()->toString() + ")";
    if (expr != nullptr) {
        ret += expr->toString();
    }

    return ret;
}
