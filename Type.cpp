#include "Type.h"

#include "llvm/Support/raw_ostream.h"

StructType::StructType(const std::string& name)
    : name(name) { }

void StructType::print() const {
    llvm::outs() << "struct " << name;
}

std::string StructType::toString() const {
    return "struct " + name;
}

ArrayType::ArrayType(std::unique_ptr<Type> type, unsigned int size)
    : size(size),
      type(std::move(type)) { }

void ArrayType::print() const {
    type->print();
}

void ArrayType::printSize() const {
    llvm::outs() << "[" << size << "]";

    if (ArrayType* AT = dynamic_cast<ArrayType*>(type.get())) {
        AT->printSize();
    }
}

std::string ArrayType::toString() const {
    return type->toString();
}

std::string ArrayType::sizeToString() const {
    std::string ret;

    ret += "[";
    ret += size;
    ret += "]";
    if (ArrayType* AT = dynamic_cast<ArrayType*>(type.get())) {
        ret += AT->sizeToString();
    }

    return ret;
}

void VoidType::print() const {
    llvm::outs() << "void";
}

std::string VoidType::toString() const {
    return "void";
}

PointerType::PointerType(std::unique_ptr<Type> type)
    : type(std::move(type)) { }

void PointerType::print() const {
    type->print();
    llvm::outs() << "*";
}

std::string PointerType::toString() const {
    return type->toString() + "*";
}

IntegerType::IntegerType(const std::string& name, bool unsignedType)
    : name(name),
      unsignedType(unsignedType) { }

void IntegerType::print() const {
    if (unsignedType) {
        llvm::outs() << "unsigned ";
    }
    llvm::outs() << name;
}

std::string IntegerType::toString() const {
    std::string ret;

    if (unsignedType) {
        ret += "unsigned ";
    }
    ret += name;

    return ret;
}

CharType::CharType(bool unsignedType)
    : IntegerType("char", unsignedType) { }

IntType::IntType(bool unsignedType)
    : IntegerType("int", unsignedType) { }

ShortType::ShortType(bool unsignedType)
    : IntegerType("short", unsignedType) { }

LongType::LongType(bool unsignedType)
    : IntegerType("long", unsignedType) { }

FloatingPointType::FloatingPointType(const std::string& name)
    :name(name) { }

void FloatingPointType::print() const {
    llvm::outs() << name;
}

std::string FloatingPointType::toString() const {
    return name;
}

FloatType::FloatType()
    : FloatingPointType("float") { }

DoubleType::DoubleType()
    : FloatingPointType("double") { }

LongDoubleType::LongDoubleType()
    : FloatingPointType("long double") { }
