#pragma once

#include <string>

#include "Expr.h"

class BinaryExpr : public Expr {
public:
    Expr* left;
    Expr* right;

    BinaryExpr(Expr*, Expr*);
};

class AddExpr : public BinaryExpr {
public:
    AddExpr(Expr*, Expr*);
    void print() const override;
    std::string toString() const override;
};

class SubExpr : public BinaryExpr {
public:
    SubExpr(Expr*, Expr*);
    void print() const override;
    std::string toString() const override;
};

class EqualsExpr : public BinaryExpr {
public:
    EqualsExpr(Expr*, Expr*);
    void print() const override;
    std::string toString() const override;
};

class MulExpr : public BinaryExpr {
public:
    MulExpr(Expr*, Expr*);
    void print() const override;
    std::string toString() const override;
};

class DivExpr : public BinaryExpr {
public:
    DivExpr(Expr*, Expr*);
    void print() const override;
    std::string toString() const override;
};

class RemExpr : public BinaryExpr {
public:
    RemExpr(Expr*, Expr*);
    void print() const override;
    std::string toString() const override;
};

class AndExpr : public BinaryExpr {
public:
    AndExpr(Expr*, Expr*);
    void print() const override;
    std::string toString() const override;
};

class OrExpr : public BinaryExpr {
public:
    OrExpr(Expr*, Expr*);
    void print() const override;
    std::string toString() const override;
};

class XorExpr : public BinaryExpr {
public:
    XorExpr(Expr*, Expr*);
    void print() const override;
    std::string toString() const override;
};

class CmpExpr : public BinaryExpr {
public:
    std::string comparsion;

    CmpExpr(Expr*, Expr*, std::string);
    void print() const override;
    std::string toString() const override;
};

class AshrExpr : public BinaryExpr {
public:
    AshrExpr(Expr*, Expr*);
    void print() const override;
    std::string toString() const override;
};

class LshrExpr : public BinaryExpr {
public:
    LshrExpr(Expr*, Expr*);
    void print() const override;
    std::string toString() const override;
};

class ShlExpr : public BinaryExpr {
public:
    ShlExpr(Expr*, Expr*);
    void print() const override;
    std::string toString() const override;
};

class SelectExpr : public BinaryExpr {
public:
    Expr* comp;

    SelectExpr(Expr*, Expr*, Expr*);
    void print() const override;
    std::string toString() const override;
};
