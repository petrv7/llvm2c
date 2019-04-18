#pragma once

#include <string>

#include "Expr.h"

/**
 * @brief The UnaryExpr class is a base class for all unary expressions.
 */
class UnaryExpr : public ExprBase {
public:
    UnaryExpr(Expr *);

    Expr* expr; //operand of unary operation
};

/**
 * @brief The RefExpr class represent reference.
 */
class RefExpr : public UnaryExpr {
public:
    RefExpr(Expr*);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The DerefExpr class represents dereference.
 */
class DerefExpr : public UnaryExpr {
public:
    DerefExpr(Expr*);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The RetExpr class represents return.
 */
class RetExpr : public UnaryExpr {
public:
    RetExpr(Expr*);
    RetExpr();

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The CastExpr class represents cast.
 */
class CastExpr : public UnaryExpr {
public:
    CastExpr(Expr*, std::unique_ptr<Type>);

    void print() const override;
    std::string toString() const override;
};
