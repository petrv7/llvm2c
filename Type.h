#pragma once

#include "llvm/IR/Type.h"

#include <string>
#include <memory>

class Type {
public:
    virtual ~Type() = default;
    virtual void print() const = 0;
    virtual std::string toString() const = 0;

    /**
     * @brief getType Transforms llvm::Type into corresponding Type object
     * @param type llvm::Type for transformation
     * @param isArray Indicates that given llvm::Type is array
     * @param size Size of the array
     * @return unique_ptr to corresponding Type object
     */
    static std::unique_ptr<Type> getType(const llvm::Type* type, bool isArray = false, unsigned int size = 0);
};

class StructType : public Type {
public:
    std::string name;

    StructType(const std::string&);

    void print() const override;
    std::string toString() const override;
};

class ArrayType : public Type {
public:
    unsigned int size;
    std::unique_ptr<Type> type;

    ArrayType(std::unique_ptr<Type>, unsigned int);

    void print() const override;
    void printSize() const;
    std::string toString() const override;
    std::string sizeToString() const;
};

class VoidType : public Type {
public:
    void print() const override;
    std::string toString() const override;
};

class PointerType : public Type {
public:
    std::unique_ptr<Type> type;

    PointerType(std::unique_ptr<Type>);

    void print() const override;
    std::string toString() const override;
};

class IntegerType : public Type {
public:
    std::string name;
    bool unsignedType;

    IntegerType(const std::string&, bool);

    void print() const override;
    std::string toString() const override;
};

class CharType : public IntegerType {
public:
    CharType(bool);
};

class IntType : public IntegerType {
public:
    IntType(bool);
};

class ShortType : public IntegerType {
public:
    ShortType(bool);
};

class LongType : public IntegerType {
public:
    LongType(bool);
};

class FloatingPointType : public Type {
public:
    std::string name;

    FloatingPointType(const std::string&);
    void print() const override;
    std::string toString() const override;
};

class FloatType : public FloatingPointType {
public:
    FloatType();
};

class DoubleType : public FloatingPointType {
public:
    DoubleType();
};

class LongDoubleType : public FloatingPointType {
public:
    LongDoubleType();
};
