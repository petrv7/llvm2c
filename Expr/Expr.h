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
    unsigned element;

public:
    StructElement(Struct*, Expr*, unsigned);

    void print() const override;
    std::string toString() const override;
};

class ArrayElement : public ExprBase {
private:
    Expr* expr;
    Expr* element;

public:
    ArrayElement(Expr*, Expr*);
    ArrayElement(Expr*, Expr*, std::unique_ptr<Type>);

    void print() const override;
    std::string toString() const override;
};

class ExtractValueExpr : public ExprBase {
private:
    std::vector<std::unique_ptr<Expr>> indices;

public:
    ExtractValueExpr(std::vector<std::unique_ptr<Expr>>&);

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
    std::string inst; //asm string
    std::vector<std::pair<std::string, Expr*>> output; //output constraints
    std::vector<std::pair<std::string, Expr*>> input; //input constraints
    std::string clobbers;

public:
    AsmExpr(const std::string&, const std::vector<std::pair<std::string, Expr*>>&, const std::vector<std::pair<std::string, Expr*>>&, const std::string&);

    void print() const override;
    std::string toString() const override;

    /**
     * @brief addOutputExpr Adds Expr to the output vector
     * @param expr Output expression
     * @param pos Position in vector
     */
    void addOutputExpr(Expr* expr, unsigned pos);
};

class CallExpr : public ExprBase {
private:
    Expr* funcValue;
    std::string funcName;
    std::vector<Expr*> params;

public:
    CallExpr(Expr*, const std::string&, std::vector<Expr*>, std::unique_ptr<Type>);

    void print() const override;
    std::string toString() const override;
};

class PointerMove : public ExprBase {
private:
    std::unique_ptr<Type> ptrType;
    Expr* pointer;
    Expr* move;

public:
    PointerMove(std::unique_ptr<Type>, Expr*, Expr*);

    void print() const override;
    std::string toString() const override;
};

class GepExpr : public ExprBase {
private:
    std::vector<std::unique_ptr<Expr>> indices;

public:
    GepExpr(std::vector<std::unique_ptr<Expr>>&);

    void print() const override;
    std::string toString() const override;
};
