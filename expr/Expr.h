#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

#include "llvm/Support/raw_ostream.h"

#include "../type/Type.h"

/**
 * @brief The Expr class is an abstract class for all expressions.
 */
class Expr {
public:
    virtual ~Expr() = default;
    virtual void print() const = 0;
    virtual std::string toString() const = 0;
    virtual Type* getType() = 0;
    virtual void setType(std::unique_ptr<Type>) = 0;
};

/**
 * @brief The ExprBase class is a base class for all expressions. It contains type of the expression and function to get/set the type.
 */
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

/**
 * @brief The Struct class represents struct with all the information needed for using it.
 */
class Struct : public ExprBase {
public:
    std::string name;
    std::vector<std::pair<std::unique_ptr<Type>, std::string>> items; //elements of the struct

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

/**
 * @brief The StructElement class represents access to element of a struct.
 */
class StructElement : public ExprBase {
private:
    Struct* strct; //struct being accessed
    Expr* expr; //expression being accessed
    unsigned element; //number of the element

public:
    StructElement(Struct*, Expr*, unsigned);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The ArrayElement class represents access to element of an array.
 */
class ArrayElement : public ExprBase {
private:
    Expr* expr; //expression being accessed
    Expr* element; //expression representing index of the element

public:
    ArrayElement(Expr*, Expr*);
    ArrayElement(Expr*, Expr*, std::unique_ptr<Type>);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The ExtractValueExpr class represents extractvalue instruction in C. It is made of sequence of StructElement and ArrayElements expressions.
 */
class ExtractValueExpr : public ExprBase {
private:
    std::vector<std::unique_ptr<Expr>> indices; //sequence of StructElement and ArrayElements expressions

public:
    ExtractValueExpr(std::vector<std::unique_ptr<Expr>>&);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The Value class represents variable or constant value.
 */
class Value : public ExprBase {
public:
    std::string valueName;
    bool init; //used for declaration printing

    Value(const std::string&, std::unique_ptr<Type>);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The GlobalValue class represents global variable.
 */
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

/**
 * @brief The IfExpr class represents br instruction in C as an if-else statement.
 */
class IfExpr : public ExprBase {
private:
    Expr* cmp; //expression used as a condition
    std::string trueBlock; //goto trueBlock if condition is true
    std::string falseBlock; //else goto falseBlock

public:
    IfExpr(Expr*, const std::string&, const std::string&);
    IfExpr(const std::string& trueBlock);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The SwitchExpr class represents switch.
 */
class SwitchExpr : public ExprBase {
private:
    Expr* cmp; //expression used in switch
    std::string def; //default
    std::map<int, std::string> cases; //cases of switch

public:
    SwitchExpr(Expr*, const std::string&, std::map<int, std::string>);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The AsmExpr class represents calling of inline asm.
 */
class AsmExpr : public ExprBase {
private:
    std::string inst; //asm string
    std::vector<std::pair<std::string, Expr*>> output; //output constraints
    std::vector<std::pair<std::string, Expr*>> input; //input constraints
    std::string clobbers; //clobber

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

/**
 * @brief The CallExpr class represents function call.
 */
class CallExpr : public ExprBase {
private:
    Expr* funcValue; //expression in case of calling function pointer
    std::string funcName; //name of the called function
    std::vector<Expr*> params; //parameters of the function call

public:
    CallExpr(Expr*, const std::string&, std::vector<Expr*>, std::unique_ptr<Type>);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The PointerMove class represents moving of a pointer.
 */
class PointerMove : public ExprBase {
private:
    std::unique_ptr<Type> ptrType; //type of the pointer
    Expr* pointer; //expression being moved
    Expr* move; //expression representing number used for moving

public:
    PointerMove(std::unique_ptr<Type>, Expr*, Expr*);

    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The GepExpr class represents getelementptr instruction in C. It is made of sequence of StructElement, ArrayElement and PointerMove expressions.
 */
class GepExpr : public ExprBase {
private:
    std::vector<std::unique_ptr<Expr>> indices; //sequence of StructElement, ArrayElement and PointerMove expressions

public:
    GepExpr(std::vector<std::unique_ptr<Expr>>&);

    void print() const override;
    std::string toString() const override;
};
