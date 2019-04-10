#pragma once

#include "llvm/IR/Type.h"

#include <string>
#include <memory>

class Type {
public:
    virtual ~Type() = default;
    virtual std::unique_ptr<Type> clone() const = 0;
    virtual void print() const = 0;
    virtual std::string toString() const = 0;

    bool isConst = false;
    bool isStatic = false;

    std::string getConstStaticString() const {
        std::string ret;

        if (isConst) {
            ret += "const ";
        }
        if (isStatic) {
            ret += "static ";
        }

        return ret;
    }
};

class TypeDef : public Type {
private:
    std::string type;
    std::string name;
    std::string typeEnd;

    std::unique_ptr<Type> derefType;

public:
    TypeDef(std::unique_ptr<Type>, const std::string&, const std::string&, const std::string& = "");
    TypeDef(const TypeDef&);

    std::unique_ptr<Type> clone() const override;
    void print() const override;
    std::string toString() const override;

    std::unique_ptr<Type> getDerefType() {
        return derefType->clone();
    }

    std::string defToString() const;
};

class FunctionType : public Type {
friend class TypeHandler;
private:
    std::unique_ptr<Type> retType;
    std::vector<std::unique_ptr<Type>> params;

public:
    bool isVarArg = false;

    FunctionType(std::unique_ptr<Type>);
    FunctionType(const FunctionType&);

    std::unique_ptr<Type> clone() const override;
    void print() const override;
    std::string toString() const override;

    void addParam(std::unique_ptr<Type>);
    void printParams() const;
    std::string paramsToString() const;
};

class StructType : public Type {
public:
    std::string name;

    StructType(const std::string&);
    StructType(const StructType&);

    std::unique_ptr<Type> clone() const override;
    void print() const override;
    std::string toString() const override;
};

class UnnamedStructType : public Type {
public:
    std::string structString;

    UnnamedStructType(const std::string&);
    UnnamedStructType(const UnnamedStructType&);

    std::unique_ptr<Type> clone() const override;
    void print() const override;
    std::string toString() const override;
};

class PointerType : public Type {
public:
    std::unique_ptr<Type> type;
    unsigned levels;

    bool isArrayPointer;
    std::string sizes;

    bool isStructPointer;
    std::string structName;

    PointerType(std::unique_ptr<Type>);
    PointerType(const PointerType& other);

    std::unique_ptr<Type> clone() const override;
    void print() const override;
    std::string toString() const override;
};

class ArrayType : public Type {
public:
    std::unique_ptr<Type> type;
    unsigned int size;

    bool isStructArray;
    std::string structName;

    bool isPointerArray;
    PointerType* pointer;

    ArrayType(std::unique_ptr<Type>, unsigned int);
    ArrayType(const ArrayType&);

    std::unique_ptr<Type> clone() const override;
    void print() const override;
    std::string toString() const override;

    void printSize() const;
    std::string sizeToString() const;
};

class VoidType : public Type {
public:
    std::unique_ptr<Type> clone() const override;
    void print() const override;
    std::string toString() const override;
};

class IntegerType : public Type {
private:
    std::string name;

public:
    bool unsignedType;

    IntegerType(const std::string&, bool);
    IntegerType(const IntegerType&);

    std::unique_ptr<Type> clone() const override;
    void print() const override;
    std::string toString() const override;
};

class CharType : public IntegerType {
public:
    CharType(bool);

    std::unique_ptr<Type> clone() const override;
};

class IntType : public IntegerType {
public:
    IntType(bool);

    std::unique_ptr<Type> clone() const override;
};

class ShortType : public IntegerType {
public:
    ShortType(bool);

    std::unique_ptr<Type> clone() const override;
};

class LongType : public IntegerType {
public:
    LongType(bool);

    std::unique_ptr<Type> clone() const override;
};

class Int128 : public IntegerType {
public:
    Int128();

    std::unique_ptr<Type> clone() const override;
};

class FloatingPointType : public Type {
private:
    std::string name;

public:
    FloatingPointType(const std::string&);
    FloatingPointType(const FloatingPointType&);

    std::unique_ptr<Type> clone() const override;
    void print() const override;
    std::string toString() const override;
};

class FloatType : public FloatingPointType {
public:
    FloatType();

    std::unique_ptr<Type> clone() const override;
};

class DoubleType : public FloatingPointType {
public:
    DoubleType();

    std::unique_ptr<Type> clone() const override;
};

class LongDoubleType : public FloatingPointType {
public:
    LongDoubleType();

    std::unique_ptr<Type> clone() const override;
};
