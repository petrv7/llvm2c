#include "Type.h"

#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/raw_ostream.h"

FunctionType::FunctionType(std::unique_ptr<Type> retType)
    : retType(std::move(retType)) { }

FunctionType::FunctionType(const FunctionType& other) {
    retType = other.retType->clone();
    for (auto& param : other.params) {
        params.push_back(param->clone());
    }
}

void FunctionType::addParam(std::unique_ptr<Type> param) {
    params.push_back(std::move(param));
}

void FunctionType::printParams() const {
    llvm::outs() << paramsToString();
}

std::string FunctionType::paramsToString() const {
    std::string ret = "(";
    bool first = true;
    for (const auto& param : params) {
        if (!first) {
            ret += ", ";
        }
        first = false;

        ret += param->toString();
    }

    if (isVarArg) {
        ret += ", ...";
    }

    return ret + ")";
}

std::unique_ptr<Type> FunctionType::clone() const {
    return std::make_unique<FunctionType>(*this);
}

void FunctionType::print() const {
    llvm::outs() << toString();
}

std::string FunctionType::toString() const {
    std::string ret = getConstStaticString();

    return ret + retType->toString();
}

StructType::StructType(const std::string& name)
    : name(name) { }

StructType::StructType(const StructType& other) {
    name = other.name;
}

std::unique_ptr<Type> StructType::clone() const  {
    return std::make_unique<StructType>(*this);
}

void StructType::print() const {
    llvm::outs() << toString();
}

std::string StructType::toString() const {
    std::string ret = getConstStaticString();

    return ret + "struct " + name;
}

UnnamedStructType::UnnamedStructType(const std::string& structString)
    : structString(structString) { }

UnnamedStructType::UnnamedStructType(const UnnamedStructType& other) {
    structString = other.structString;
}

std::unique_ptr<Type> UnnamedStructType::clone() const {
    return std::make_unique<UnnamedStructType>(structString);
}

void UnnamedStructType::print() const {
    llvm::outs() << toString();
}

std::string UnnamedStructType::toString() const {
    return structString;
}

ArrayType::ArrayType(std::unique_ptr<Type> type, unsigned int size)
    : type(std::move(type)),
      size(size) {
    isStructArray = false;
    isPointerArray = false;

    if (auto AT = dynamic_cast<ArrayType*>(this->type.get())) {
        isStructArray = AT->isStructArray;
        structName = AT->structName;

        isPointerArray = AT->isPointerArray;
        pointer = AT->pointer;
    }

    if (auto ST = dynamic_cast<StructType*>(this->type.get())) {
        isStructArray = true;
        structName = ST->name;
    }

    if (auto PT = dynamic_cast<PointerType*>(this->type.get())) {
        isPointerArray = true;
        pointer = PT;
    }
}

ArrayType::ArrayType(const ArrayType& other) {
    size = other.size;
    type = other.type->clone();
    isStructArray = other.isStructArray;
    structName = other.structName;
    isPointerArray = other.isPointerArray;
    pointer = other.pointer;
}

std::unique_ptr<Type> ArrayType::clone() const  {
    return std::make_unique<ArrayType>(*this);
}

void ArrayType::print() const {
    type->print();
}

void ArrayType::printSize() const {
    llvm::outs() << sizeToString();
}

std::string ArrayType::toString() const {
    std::string ret = getConstStaticString();

    return ret + type->toString();
}

std::string ArrayType::sizeToString() const {
    std::string ret;

    ret += "[";
    ret += std::to_string(size);
    ret += "]";
    if (ArrayType* AT = dynamic_cast<ArrayType*>(type.get())) {
        ret += AT->sizeToString();
    }

    return ret;
}

std::unique_ptr<Type> VoidType::clone() const  {
    return std::make_unique<VoidType>();
}

void VoidType::print() const {
    llvm::outs() << toString();
}

std::string VoidType::toString() const {
    return "void";
}

PointerType::PointerType(std::unique_ptr<Type> type) {
    levels = 1;
    isFuncPointer = false;
    isArrayPointer = false;
    isStructPointer = false;

    if (auto PT = dynamic_cast<PointerType*>(type.get())) {
        isFuncPointer = PT->isFuncPointer;
        isArrayPointer = PT->isArrayPointer;
        isStructPointer = PT->isStructPointer;
        structName = PT->structName;
        levels = PT->levels + 1;
        params = PT->params;
    }

    if (auto FT = dynamic_cast<FunctionType*>(type.get())) {
        isFuncPointer = true;
        params = FT->paramsToString();
    }

    if (auto AT = dynamic_cast<ArrayType*>(type.get())) {
        isArrayPointer = true;
        size = AT->size;

        isStructPointer = AT->isStructArray;
        structName = AT->structName;
    }

    if (auto ST = dynamic_cast<StructType*>(type.get())) {
        isStructPointer = true;
        structName = ST->name;
    }

    this->type = type->clone();
}

PointerType::PointerType(const PointerType &other) {
    type = other.type->clone();
    isFuncPointer = other.isFuncPointer;
    isArrayPointer = other.isArrayPointer;
    levels = other.levels;
    params = other.params;
    size = other.size;
}

std::unique_ptr<Type> PointerType::clone() const  {
    return std::make_unique<PointerType>(*this);
}

void PointerType::print() const {
    llvm::outs() << toString();
}

std::string PointerType::toString() const {
    std::string ret = getConstStaticString();

    if (isFuncPointer || isArrayPointer) {
        return ret + type->toString();
    }

    return ret + type->toString() + "*";
}

IntegerType::IntegerType(const std::string& name, bool unsignedType)
    : name(name),
      unsignedType(unsignedType) { }

IntegerType::IntegerType(const IntegerType& other) {
    name = other.name;
    unsignedType = other.unsignedType;
}

std::unique_ptr<Type> IntegerType::clone() const  {
    return std::make_unique<IntegerType>(*this);
}

void IntegerType::print() const {
    llvm::outs() << toString();
}

std::string IntegerType::toString() const {
    std::string ret = getConstStaticString();

    if (unsignedType) {
        ret += "unsigned ";
    }
    ret += name;

    return ret;
}

CharType::CharType(bool unsignedType)
    : IntegerType("char", unsignedType) { }

std::unique_ptr<Type> CharType::clone() const  {
    return std::make_unique<CharType>(*this);
}

IntType::IntType(bool unsignedType)
    : IntegerType("int", unsignedType) { }

std::unique_ptr<Type> IntType::clone() const  {
    return std::make_unique<IntType>(*this);
}

ShortType::ShortType(bool unsignedType)
    : IntegerType("short", unsignedType) { }

std::unique_ptr<Type> ShortType::clone() const  {
    return std::make_unique<ShortType>(*this);
}

LongType::LongType(bool unsignedType)
    : IntegerType("long", unsignedType) { }

std::unique_ptr<Type> LongType::clone() const  {
    return std::make_unique<LongType>(*this);
}

UInt128::UInt128()
    : IntegerType("__uint128_t", false) { }

std::unique_ptr<Type> UInt128::clone() const {
    return std::make_unique<UInt128>();
}

FloatingPointType::FloatingPointType(const std::string& name)
    :name(name) { }

FloatingPointType::FloatingPointType(const FloatingPointType& other) {
    name = other.name;
}

std::unique_ptr<Type> FloatingPointType::clone() const  {
    return std::make_unique<FloatingPointType>(*this);
}

void FloatingPointType::print() const {
    llvm::outs() << toString();
}

std::string FloatingPointType::toString() const {
    std::string ret = getConstStaticString();

    return ret + name;
}

FloatType::FloatType()
    : FloatingPointType("float") { }

std::unique_ptr<Type> FloatType::clone() const  {
    return std::make_unique<FloatType>(*this);
}

DoubleType::DoubleType()
    : FloatingPointType("double") { }

std::unique_ptr<Type> DoubleType::clone() const  {
    return std::make_unique<DoubleType>(*this);
}

LongDoubleType::LongDoubleType()
    : FloatingPointType("long double") { }

std::unique_ptr<Type> LongDoubleType::clone() const  {
    return std::make_unique<LongDoubleType>(*this);
}
