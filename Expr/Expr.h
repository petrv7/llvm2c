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
