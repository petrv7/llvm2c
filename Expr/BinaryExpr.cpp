#include "BinaryExpr.h"

#include "llvm/Support/raw_ostream.h"

/*
 * BinaryExpr classes
 */

BinaryExpr::BinaryExpr(Expr* l, Expr* r) {
    left = l;
    right = r;
}

AddExpr::AddExpr(Expr* l, Expr* r) :
    BinaryExpr(l, r) { }

void AddExpr::print() const {
    llvm::outs() << toString();
}

std::string AddExpr::toString() const {
    return left->toString() + " + " + right->toString();
}

SubExpr::SubExpr(Expr* l, Expr* r) :
    BinaryExpr(l, r) { }

void SubExpr::print() const {
    llvm::outs() << toString();
}

std::string SubExpr::toString() const {
    return left->toString() + " - " + right->toString();
}

EqualsExpr::EqualsExpr(Expr* l, Expr* r) :
    BinaryExpr(l, r) { }

void EqualsExpr::print() const {
    llvm::outs() << toString();
}

std::string EqualsExpr::toString() const {
    return left->toString() + " = " + right->toString() + ";";
}

MulExpr::MulExpr(Expr* l, Expr* r) :
    BinaryExpr(l, r) { }

void MulExpr::print() const {
    llvm::outs() << toString();
}

std::string MulExpr::toString() const {
    return left->toString() + " * " + right->toString();
}

DivExpr::DivExpr(Expr* l, Expr* r) :
    BinaryExpr(l, r) { }

void DivExpr::print() const {
    llvm::outs() << toString();
}

std::string DivExpr::toString() const {
    return left->toString() + " / " + right->toString();
}

RemExpr::RemExpr(Expr* l, Expr* r) :
    BinaryExpr(l, r) { }

void RemExpr::print() const {
    llvm::outs() << toString();
}

std::string RemExpr::toString() const {
    return left->toString() + " % " + right->toString();
}

AndExpr::AndExpr(Expr* l, Expr* r) :
    BinaryExpr(l, r) { }

void AndExpr::print() const {
    llvm::outs() << toString();
}

std::string AndExpr::toString() const {
    return left->toString() + " & " + right->toString();
}

OrExpr::OrExpr(Expr* l, Expr* r) :
    BinaryExpr(l, r) { }

void OrExpr::print() const {
    llvm::outs() << toString();
}

std::string OrExpr::toString() const {
    return left->toString() + " | " + right->toString();
}

XorExpr::XorExpr(Expr* l, Expr* r) :
    BinaryExpr(l, r) { }

void XorExpr::print() const {
    llvm::outs() << toString();
}

std::string XorExpr::toString() const {
    return left->toString() + " ^ " + right->toString();
}

CmpExpr::CmpExpr(Expr* l, Expr* r, std::string cmp) :
    BinaryExpr(l,r) {
    comparsion = cmp;
}

void CmpExpr::print() const {
    llvm::outs() << toString();
}

std::string CmpExpr::toString() const {
    return left->toString() + " " + comparsion + " " + right->toString();
}

AshrExpr::AshrExpr(Expr* l, Expr* r) :
    BinaryExpr(l, r) { }

void AshrExpr::print() const {
    llvm::outs() << toString();
}

std::string AshrExpr::toString() const {
    return left->toString() + " >> " + right->toString();
}

//TODO
LshrExpr::LshrExpr(Expr* l, Expr* r) :
    BinaryExpr(l, r) { }

void LshrExpr::print() const {
    llvm::outs() << toString();
}

std::string LshrExpr::toString() const {
    return left->toString() + " >> " + right->toString();
}

ShlExpr::ShlExpr(Expr* l, Expr* r) :
    BinaryExpr(l, r) { }

void ShlExpr::print() const {
    llvm::outs() << toString();
}

std::string ShlExpr::toString() const {
    return left->toString() + " << " + right->toString();
}

SelectExpr::SelectExpr(Expr* comp, Expr* l, Expr* r) :
    BinaryExpr(l, r),
    comp(comp) { }

void SelectExpr::print() const {
    llvm::outs() << toString();
}

std::string SelectExpr::toString() const {
    return "(" + comp->toString() + ") ? " + left->toString() + " : " + right->toString();
}