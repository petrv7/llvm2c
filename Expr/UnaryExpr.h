#pragma once

#include <string>

#include "Expr.h"

class UnaryExpr : public Expr {
public:
    UnaryExpr(Expr *);

    Expr* expr;
};

class RefExpr : public UnaryExpr {
public:
    RefExpr(Expr*);
    void print() const override;
    std::string toString() const override;
};

class DerefExpr : public UnaryExpr {
public:
    DerefExpr(Expr*);
    void print() const override;
    std::string toString() const override;
};

class RetExpr : public UnaryExpr {
public:
    RetExpr(Expr*);
    RetExpr();
    void print() const override;
    std::string toString() const override;
};

class CastExpr : public UnaryExpr {
public:
    std::unique_ptr<Type> castType;

    CastExpr(Expr*, std::unique_ptr<Type>);
    void print() const override;
    std::string toString() const override;
};
