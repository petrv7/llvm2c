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

    /**
     * @brief getType Transforms llvm::Type into corresponding Type object
     * @param type llvm::Type for transformation
     * @return unique_ptr to corresponding Type object
     */
    static std::unique_ptr<Type> getType(const llvm::Type* type, bool voidType = false);

    /**
     * @brief getBinaryType Returns type that would be result of a binary operation
     * @param left left argument of the operation
     * @param right right argument of the operation
     * @return unique_ptr to Type object
     */
    static std::unique_ptr<Type> getBinaryType(const Type* left, const Type* right);
};

class FunctionType : public Type {
private:
    std::unique_ptr<Type> retType;
    std::vector<std::unique_ptr<Type>> params;

public:
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

class ArrayType : public Type {
public:
    std::unique_ptr<Type> type;
    unsigned int size;

    bool isStructArray;
    std::string structName;

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

class PointerType : public Type {
public:
    std::unique_ptr<Type> type;
    unsigned levels;

    bool isArrayPointer;
    unsigned int size;

    bool isStructPointer;
    std::string structName;

    bool isFuncPointer;
    std::string params;

    PointerType(std::unique_ptr<Type>);
    PointerType(const PointerType& other);

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