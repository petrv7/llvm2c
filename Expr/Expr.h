#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

#include "../Type.h"

class Expr {
public:
    virtual ~Expr() = default;
    virtual void print() const = 0;
    virtual std::string toString() const = 0;
};

class GepExpr : public Expr {
public:
    Expr* element;
    std::vector<std::pair<std::unique_ptr<Type>, std::string>> args;

    GepExpr(Expr*);
    void print() const override;
    std::string toString() const override;
    void addArg(std::unique_ptr<Type>, const std::string&);
};
/*
class GepExpr2 : public Expr {
public:
    Expr* element;
    std::vector<std::unique_ptr<Expr>> args;

    GepExpr2(Expr*);
    void print() const override;
    std::string toString() const override;
    void addArg(std::unique_ptr<Expr>);
};


class GepPointerExpr : public Expr {
    Expr* expr;
    std::string idx;

public:
    GepPointerExpr(Expr*, const std::string&);
    void print() const override;
    std::string toString() const override;
};

class GepArrayExpr : public Expr {
    Expr* expr;
    std::string idx;

public:
    GepArrayExpr(Expr*, const std::string&);
    void print() const override;
    std::string toString() const override;
};

class GepStructExpr : public Expr {
    Expr* expr;
    std::string element;

public:
    GepStructExpr(Expr*, const std::string&);
    void print() const override;
    std::string toString() const override;
};
*/
class Struct : public Expr {
public:
    std::string name;
    std::vector<std::pair<std::unique_ptr<Type>, std::string>> items;

    Struct(const std::string&);
    void print() const override;
    std::string toString() const override;
    void addItem(std::unique_ptr<Type>, const std::string&);
};

class Value : public Expr {
public:
    std::unique_ptr<Type> type;
    std::string val;
    bool typePrinted;

    Value(const std::string&, std::unique_ptr<Type>);
    void print() const override;
    std::string toString() const override;
};

class JumpExpr : public Expr {
public:
    JumpExpr(const std::string&);
    std::string block;
    void print() const override;
    std::string toString() const override;
};

class IfExpr : public Expr {
public:
    Expr* cmp;
    std::string trueBlock;
    std::string falseBlock;

    IfExpr(Expr*, const std::string&, const std::string&);
    IfExpr(const std::string& trueBlock);
    void print() const override;
    std::string toString() const override;
};

class SwitchExpr : public Expr {
public:
    Expr* cmp;
    std::string def;
    std::map<int, std::string> cases;

    SwitchExpr(Expr*, const std::string&, std::map<int, std::string>);
    void print() const override;
    std::string toString() const override;
};

class AsmExpr : public Expr { //rename
public:
    std::string inst;

    AsmExpr(const std::string&);
    void print() const override;
    std::string toString() const override;
};

class CallExpr : public Expr {
public:
    std::string funcName;
    std::vector<Expr*> params;

    CallExpr(const std::string&, std::vector<Expr*>);
    void print() const override;
    std::string toString() const override;
};
