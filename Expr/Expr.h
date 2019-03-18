#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

#include "llvm/Support/raw_ostream.h"

#include "../Type/Type.h"

class Expr {
public:
    virtual ~Expr() = default;
    virtual void print() const = 0;
    virtual std::string toString() const = 0;
    virtual Type* getType() = 0;
    virtual void setType(std::unique_ptr<Type>) = 0;
};

class ExprBase : public Expr {
private:
    std::unique_ptr<Type> type;

public:
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

    bool isPrinted; //used for printing structs in the right order

    Struct(const std::string&);

    void print() const override;
    std::string toString() const override;

    /**
     * @brief addItem Adds new struct element to the vector items.
     * @param type Type of the element
     * @param name Name of the element
     */
    void addItem(std::unique_ptr<Type> type, const std::string& name);
};

class StructElement : public ExprBase {
private:
    Struct* strct;
    Expr* expr;
    long element;
    unsigned int move;

public:
    StructElement(Struct*, Expr*, long, unsigned int move);

    void print() const override;
    std::string toString() const override;
};

class Value : public ExprBase {
public:
    std::string valueName;
    bool init; //used for declaration printing

    Value(const std::string&, std::unique_ptr<Type>);

    void print() const override;
    std::string toString() const override;
};

class GlobalValue : public Value {
private:
    std::string value;

public:
    GlobalValue(const std::string&, const std::string&, std::unique_ptr<Type>);

    void print() const override;
    std::string toString() const override;

    /**
     * @brief declToString Returns string containing declaration only.
     * @return String containing declaration of the global variable;
     */
    std::string declToString() const;
};

class JumpExpr : public ExprBase {
private:
    std::string block;

public:
    JumpExpr(const std::string&);

    void print() const override;
    std::string toString() const override;
};

class IfExpr : public ExprBase {
private:
    Expr* cmp;
    std::string trueBlock;
    std::string falseBlock;

public:
    IfExpr(Expr*, const std::string&, const std::string&);
    IfExpr(const std::string& trueBlock);

    void print() const override;
    std::string toString() const override;
};

class SwitchExpr : public ExprBase {
private:
    Expr* cmp;
    std::string def;
    std::map<int, std::string> cases;

public:
    SwitchExpr(Expr*, const std::string&, std::map<int, std::string>);

    void print() const override;
    std::string toString() const override;
};

class AsmExpr : public ExprBase {
private:
    std::string inst;

public:
    AsmExpr(const std::string&);

    void print() const override;
    std::string toString() const override;
};

class CallExpr : public ExprBase {
private:
    std::string funcName;
    std::vector<Expr*> params;
    bool isFuncPointer;

public:
    CallExpr(const std::string&, std::vector<Expr*>, std::unique_ptr<Type>, bool);

    void print() const override;
    std::string toString() const override;
};
