#pragma once

#include <string>

#include "Expr.h"

class UnaryExpr : public ExprBase {
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

class GepExpr : public UnaryExpr {
private:
    std::vector<std::pair<std::unique_ptr<Type>, std::string>> args; //vector containing pairs of type of the pointer and a string containing an increment of the pointer

public:
    GepExpr(Expr*, std::unique_ptr<Type>);

    void print() const override;
    std::string toString() const override;

    /**
     * @brief addArg Adds new pair to the vector args.
     * @param type Type of the pointer
     * @param index Increment of the pointer
     */
    void addArg(std::unique_ptr<Type>type , const std::string& index);
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
    CastExpr(Expr*, std::unique_ptr<Type>);

    void print() const override;
    std::string toString() const override;
};
