#include "Type.h"

#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/raw_ostream.h"

std::unique_ptr<Type> Type::getType(const llvm::Type* type) {
    if (type->isArrayTy()) {
        return std::make_unique<ArrayType>(std::move(getType(type->getArrayElementType())), type->getArrayNumElements());
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

    if (type->isFunctionTy()) {
        const llvm::FunctionType* FT = llvm::cast<llvm::FunctionType>(type);
        auto functionType = std::make_unique<FunctionType>(std::move(getType(FT->getReturnType())));
        for (unsigned i = 0; i < FT->getNumParams(); i++) {
            functionType->addParam(std::move(getType(FT->getParamType(0))));
        }

        return functionType;
    }

    return nullptr;
}

FunctionType::FunctionType(std::unique_ptr<Type> retType)
    : retType(std::move(retType)) { }

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

    return ret + ")";
}

void FunctionType::print() const {
    llvm::outs() << toString();
}

std::string FunctionType::toString() const {
    return retType->toString();
}

StructType::StructType(const std::string& name)
    : name(name) { }

void StructType::print() const {
    llvm::outs() << toString();
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
    llvm::outs() << sizeToString();
}

std::string ArrayType::toString() const {
    return type->toString();
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

void VoidType::print() const {
    llvm::outs() << toString();
}

std::string VoidType::toString() const {
    return "void";
}

PointerType::PointerType(std::unique_ptr<Type> type) {
    if (auto PT = dynamic_cast<PointerType*>(type.get())) {
        isFuncPointer = PT->isFuncPointer;
        levels = PT->levels + 1;
        params = PT->params;
    }

    if (auto FT = dynamic_cast<FunctionType*>(type.get())) {
        isFuncPointer = true;
        params = FT->paramsToString();
    }

    levels = 1;
    this->type = std::move(type);
}

void PointerType::print() const {
    llvm::outs() << toString();
}

std::string PointerType::toString() const {
    if (isFuncPointer) {
        return type->toString();
    }

    return type->toString() + "*";
}

IntegerType::IntegerType(const std::string& name, bool unsignedType)
    : name(name),
      unsignedType(unsignedType) { }

void IntegerType::print() const {
    llvm::outs() << toString();
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
    llvm::outs() << toString();
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
