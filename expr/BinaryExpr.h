#pragma once

#include <string>

#include "Expr.h"

/**
 * @brief The BinaryExpr class is a base class for all binary expressions.
 */
class BinaryExpr : public ExprBase {
public:
    Expr* left; //first operand of binary operation
    Expr* right; //second operand of binary operation

    BinaryExpr(Expr*, Expr*);
};

/**
 * @brief The AddExpr class represents add.
 */
class AddExpr : public BinaryExpr {
public:
    AddExpr(Expr*, Expr*);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The SubExpr class represents substitution.
 */
class SubExpr : public BinaryExpr {
public:
    SubExpr(Expr*, Expr*);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The AssignExpr class represents assingment.
 */
class AssignExpr : public BinaryExpr {
public:
    AssignExpr(Expr*, Expr*);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The MulExpr class represents multiplication.
 */
class MulExpr : public BinaryExpr {
public:
    MulExpr(Expr*, Expr*);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The DivExpr class represents division.
 */
class DivExpr : public BinaryExpr {
public:
    DivExpr(Expr*, Expr*);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The RemExpr class represents remainder (modulo).
 */
class RemExpr : public BinaryExpr {
public:
    RemExpr(Expr*, Expr*);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The AndExpr class represents bitwise AND.
 */
class AndExpr : public BinaryExpr {
public:
    AndExpr(Expr*, Expr*);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The OrExpr class represents bitwise OR.
 */
class OrExpr : public BinaryExpr {
public:
    OrExpr(Expr*, Expr*);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The XorExpr class represents bitwise XOR.
 */
class XorExpr : public BinaryExpr {
public:
    XorExpr(Expr*, Expr*);
    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The CmpExpr class represents comparsion.
 */
class CmpExpr : public BinaryExpr {
private:
    std::string comparsion; //symbol of comparsion
    bool isUnsigned; //indicates that unsigned version of cmp instruction was used

public:
    CmpExpr(Expr*, Expr*, const std::string&, bool);
    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The AshrExpr class represents arithmetic shift right.
 */
class AshrExpr : public BinaryExpr {
public:
    AshrExpr(Expr*, Expr*);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The LshrExpr class represents logical shift right.
 */
class LshrExpr : public BinaryExpr {
public:
    LshrExpr(Expr*, Expr*);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The ShlExpr class represents shift left.
 */
class ShlExpr : public BinaryExpr {
public:
    ShlExpr(Expr*, Expr*);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The SelectExpr class represents select instruction in C (comp ? left : right).
 */
class SelectExpr : public BinaryExpr {
private:
    Expr* comp;

public:
    SelectExpr(Expr*, Expr*, Expr*);

    void print() const override;
    std::string toString() const override;
};
