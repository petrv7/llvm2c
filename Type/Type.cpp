#include "Type.h"

#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/raw_ostream.h"

std::unique_ptr<Type> Type::getType(const llvm::Type* type, bool voidType) {
    if (type->isArrayTy()) {
        return std::make_unique<ArrayType>(getType(type->getArrayElementType(), voidType), type->getArrayNumElements());
    }

    if (type->isVoidTy()) {
        return std::make_unique<VoidType>();
    }

    if (type->isIntegerTy()) {
        const auto intType = static_cast<const llvm::IntegerType*>(type);
        switch(intType->getBitWidth()) {
        case 8:
            if (voidType) {
                return std::make_unique<VoidType>();
            }
            return std::make_unique<CharType>(false);
        case 16:
            return std::make_unique<ShortType>(false);
        case 1:
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
        return std::make_unique<PointerType>(getType(ptr->getElementType(), voidType));
    }

    if (type->isStructTy()) {
        const llvm::StructType* structType = llvm::dyn_cast<const llvm::StructType>(type);
        if (structType->getName().str().compare("struct.__va_list_tag") == 0) {
            return std::make_unique<StructType>("__va_list_tag");
        }
        if (structType->getName().str().substr(0, 6).compare("struct") == 0) {
            return std::make_unique<StructType>(structType->getName().str().erase(0, 7));
        } else {
            //union
            return std::make_unique<StructType>(structType->getName().str().erase(0, 6));
        }
    }

    if (type->isFunctionTy()) {
        const llvm::FunctionType* FT = llvm::cast<llvm::FunctionType>(type);
        auto functionType = std::make_unique<FunctionType>(getType(FT->getReturnType(), voidType));
        if (FT->getNumParams() == 0) {
            functionType->addParam(std::make_unique<VoidType>());
        } else {
            for (unsigned i = 0; i < FT->getNumParams(); i++) {
                functionType->addParam(getType(FT->getParamType(0), voidType));
            }
        }

        return functionType;
    }

    return nullptr;
}

std::unique_ptr<Type> Type::getBinaryType(const Type* left, const Type* right) {
    if (const auto LDT = dynamic_cast<const LongDoubleType*>(left)) {
        return std::make_unique<LongDoubleType>();
    }
    if (const auto LDT = dynamic_cast<const LongDoubleType*>(right)) {
        return std::make_unique<LongDoubleType>();
    }

    if (const auto DT = dynamic_cast<const DoubleType*>(left)) {
        return std::make_unique<DoubleType>();
    }
    if (const auto DT = dynamic_cast<const DoubleType*>(right)) {
        return std::make_unique<DoubleType>();
    }

    if (const auto FT = dynamic_cast<const FloatType*>(left)) {
        return std::make_unique<FloatType>();
    }
    if (const auto FT = dynamic_cast<const FloatType*>(right)) {
        return std::make_unique<FloatType>();
    }

    if (const auto LT = dynamic_cast<const LongType*>(left)) {
        return std::make_unique<LongType>(LT->unsignedType);
    }
    if (const auto LT = dynamic_cast<const LongType*>(right)) {
        return std::make_unique<LongType>(LT->unsignedType);
    }

    if (const auto IT = dynamic_cast<const IntType*>(left)) {
        return std::make_unique<IntType>(IT->unsignedType);
    }
    if (const auto IT = dynamic_cast<const IntType*>(right)) {
        return std::make_unique<IntType>(IT->unsignedType);
    }

    if (const auto ST = dynamic_cast<const ShortType*>(left)) {
        return std::make_unique<ShortType>(ST->unsignedType);
    }
    if (const auto ST = dynamic_cast<const ShortType*>(right)) {
        return std::make_unique<ShortType>(ST->unsignedType);
    }

    if (const auto CT = dynamic_cast<const CharType*>(left)) {
        return std::make_unique<CharType>(CT->unsignedType);
    }
    if (const auto CT = dynamic_cast<const CharType*>(right)) {
        return std::make_unique<CharType>(CT->unsignedType);
    }

    return nullptr;
}

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

ArrayType::ArrayType(std::unique_ptr<Type> type, unsigned int size)
    : type(std::move(type)),
      size(size) {
    isStructArray = false;

    if (auto AT = dynamic_cast<ArrayType*>(this->type.get())) {
        isStructArray = AT->isStructArray;
        structName = AT->structName;
    }

    if (auto PT = dynamic_cast<PointerType*>(this->type.get())) {
        isStructArray = PT->isStructPointer;
        structName = PT->structName;
    }

    if (auto ST = dynamic_cast<StructType*>(this->type.get())) {
        isStructArray = true;
        structName = ST->name;
    }
}

ArrayType::ArrayType(const ArrayType& other) {
    size = other.size;
    type = other.type->clone();
    isStructArray = other.isStructArray;
    structName = other.structName;
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

    this->type = std::move(type);
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