#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

#include "llvm/Support/raw_ostream.h"

#include "../Type.h"

class Expr {
public:
    virtual ~Expr() = default;
    virtual void print() const = 0;
    virtual std::string toString() const = 0;
    virtual Type* getType() = 0;
    virtual void setType(std::unique_ptr<Type>) = 0;
};

class ExprBase : public Expr {
public:
    std::unique_ptr<Type> type;

    Type* getType() override {
        return type.get();
    }

    void setType(std::unique_ptr<Type> type) override {
        this->type = std::move(type);
    }
};

class Struct : public ExprBase {
public:
    std::string name;
    std::vector<std::pair<std::unique_ptr<Type>, std::string>> items;

    Struct(const std::string&);
    void print() const override;
    std::string toString() const override;
    void addItem(std::unique_ptr<Type>, const std::string&);
};

class Value : public ExprBase {
public:
    std::string valueName;
    bool init;

    Value(const std::string&, std::unique_ptr<Type>);
    void print() const override;
    std::string toString() const override;
};

class GlobalValue : public Value {
public:
    std::string value;

    GlobalValue(const std::string&, const std::string&, std::unique_ptr<Type>);
    void print() const override;
    std::string toString() const override;
};

class JumpExpr : public ExprBase {
public:
    JumpExpr(const std::string&);
    std::string block;
    void print() const override;
    std::string toString() const override;
};

class IfExpr : public ExprBase {
public:
    Expr* cmp;
    std::string trueBlock;
    std::string falseBlock;

    IfExpr(Expr*, const std::string&, const std::string&);
    IfExpr(const std::string& trueBlock);
    void print() const override;
    std::string toString() const override;
};

class SwitchExpr : public ExprBase {
public:
    Expr* cmp;
    std::string def;
    std::map<int, std::string> cases;

    SwitchExpr(Expr*, const std::string&, std::map<int, std::string>);
    void print() const override;
    std::string toString() const override;
};

class AsmExpr : public ExprBase {
public:
    std::string inst;

    AsmExpr(const std::string&);
    void print() const override;
    std::string toString() const override;
};

class CallExpr : public ExprBase {
public:
    std::string funcName;
    std::vector<Expr*> params;
    bool isUsed;

    CallExpr(const std::string&, std::vector<Expr*>, std::unique_ptr<Type>);
    void print() const override;
    std::string toString() const override;
};
