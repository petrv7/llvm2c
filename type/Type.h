#pragma once

#include "llvm/IR/Type.h"

#include <string>
#include <memory>

/**
 * @brief The Type class is an abstract class for all types.
 */
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

/**
 * @brief The FunctionPointerType class represents function pointer.
 * It contains all the information needed for printing the FunctionPointerType definition.
 */
class FunctionPointerType : public Type {
friend class TypeHandler;
private:
    //type is split into two string so the name can be printed separately
    std::string type;
    std::string name;
    std::string typeEnd;

public:
    FunctionPointerType(const std::string&, const std::string&, const std::string&);
    FunctionPointerType(const FunctionPointerType&);

    std::unique_ptr<Type> clone() const override;
    void print() const override;
    std::string toString() const override;

    /**
     * @brief defToString Returns definition of FunctionPointerType as a typedef string.
     * @return String with FunctionPointerType definition
     */
    std::string defToString() const;
};

/**
 * @brief The StructType class represents struct as a type.
 */
class StructType : public Type {
public:
    std::string name;

    StructType(const std::string&);
    StructType(const StructType&);

    std::unique_ptr<Type> clone() const override;
    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The PointerType class represents pointer.
 */
class PointerType : public Type {
public:
    std::unique_ptr<Type> type;
    unsigned levels; //number of pointers (for instance int** is level 2), used for easier printing

    bool isArrayPointer; //indicates whether the pointer is pointing to array
    std::string sizes; //sizes of arrays

    bool isStructPointer; //indicates whether the pointer is pointing to struct
    std::string structName; //name of the struct

    PointerType(std::unique_ptr<Type>);
    PointerType(const PointerType& other);

    std::unique_ptr<Type> clone() const override;
    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The ArrayType class represents array.
 */
class ArrayType : public Type {
public:
    std::unique_ptr<Type> type;
    unsigned int size;

    bool isStructArray; //indicates whether the array contains structs
    std::string structName; //name of the structs

    bool isPointerArray; //indicates whether the array contains pointers
    PointerType* pointer; //pointers contained in array

    ArrayType(std::unique_ptr<Type>, unsigned int);
    ArrayType(const ArrayType&);

    std::unique_ptr<Type> clone() const override;
    void print() const override;
    std::string toString() const override;

    void printSize() const;
    std::string sizeToString() const;
};

/**
 * @brief The VoidType class represents void.
 */
class VoidType : public Type {
public:
    std::unique_ptr<Type> clone() const override;
    void print() const override;
    std::string toString() const override;
};

/**
 * @brief The IntegerType class is a base class for all integer types.
 */
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

/**
 * @brief The CharType class represents char.
 */
class CharType : public IntegerType {
public:
    CharType(bool);

    std::unique_ptr<Type> clone() const override;
};

/**
 * @brief The IntType class represents int.
 */
class IntType : public IntegerType {
public:
    IntType(bool);

    std::unique_ptr<Type> clone() const override;
};

/**
 * @brief The ShortType class represents short.
 */
class ShortType : public IntegerType {
public:
    ShortType(bool);

    std::unique_ptr<Type> clone() const override;
};

/**
 * @brief The LongType class represents long.
 */
class LongType : public IntegerType {
public:
    LongType(bool);

    std::unique_ptr<Type> clone() const override;
};

/**
 * @brief The Int128 class represents __int128.
 */
class Int128 : public IntegerType {
public:
    Int128();

    std::unique_ptr<Type> clone() const override;
};

/**
 * @brief The FloatingPointType class is a base class for all floating point types.
 */
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

/**
 * @brief The FloatType class represents float.
 */
class FloatType : public FloatingPointType {
public:
    FloatType();

    std::unique_ptr<Type> clone() const override;
};

/**
 * @brief The DoubleType class represents double.
 */
class DoubleType : public FloatingPointType {
public:
    DoubleType();

    std::unique_ptr<Type> clone() const override;
};

/**
 * @brief The LongDoubleType class represents long double.
 */
class LongDoubleType : public FloatingPointType {
public:
    LongDoubleType();

    std::unique_ptr<Type> clone() const override;
};
