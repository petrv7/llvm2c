#include "Type.h"

#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/raw_ostream.h"

std::unique_ptr<Type> Type::getType(const llvm::Type* type, bool isArray, unsigned int size) {
    if (isArray) {
        if (type->getArrayElementType()->isArrayTy()) {
            return std::make_unique<ArrayType>(std::move(getType(type->getArrayElementType(), true, type->getArrayElementType()->getArrayNumElements())), size);
        } else {
            return std::make_unique<ArrayType>(std::move(getType(type->getArrayElementType())), size);
        }
    }

    if (type->isVoidTy()) {
        return std::make_unique<VoidType>();
    }

    if (type->isIntegerTy()) {
        const auto intType = static_cast<const llvm::IntegerType*>(type);
        switch(intType->getBitWidth()) {
        case 8:
            return std::make_unique<CharType>(false);
        case 16:
            return std::make_unique<ShortType>(false);
        case 32:
            return std::make_unique<IntType>(false);
        case 64:
            return std::make_unique<LongType>(false);
        default:
            return nullptr;
        }
    }

    if (type->isFloatTy()) {
        return std::make_unique<FloatType>();
    }

    if (type->isDoubleTy()) {
        return std::make_unique<DoubleType>();
    }

    if (type->isX86_FP80Ty()) {
        return std::make_unique<LongDoubleType>();
    }

    if (type->isPointerTy()) {
        const auto ptr = static_cast<const llvm::PointerType*>(type);
        return std::make_unique<PointerType>(std::move(getType(ptr->getElementType())));
    }

    if (type->isStructTy()) {
        const llvm::StructType* structType = llvm::dyn_cast<const llvm::StructType>(type);
        return std::make_unique<StructType>(structType->getName().str().erase(0, 7));
    }

    return nullptr;
}

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
