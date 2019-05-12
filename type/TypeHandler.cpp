#include "TypeHandler.h"

#include "llvm/IR/DerivedTypes.h"

#include "../core/Program.h"

#include <boost/lambda/lambda.hpp>

std::unique_ptr<Type> TypeHandler::getType(const llvm::Type* type) {
    if (typeDefs.find(type) != typeDefs.end()) {
        return typeDefs[type]->clone();
    }

    if (type->isArrayTy()) {
        return std::make_unique<ArrayType>(getType(type->getArrayElementType()), type->getArrayNumElements());
    }

    if (type->isVoidTy()) {
        return std::make_unique<VoidType>();
    }

    if (type->isIntegerTy()) {
        const auto intType = static_cast<const llvm::IntegerType*>(type);
        if (intType->getBitWidth() == 1) {
            return std::make_unique<IntType>(false);
        }

        if (intType->getBitWidth() <= 8) {
            return std::make_unique<CharType>(false);
        }

        if (intType->getBitWidth() <= 16) {
            return std::make_unique<ShortType>(false);
        }

        if (intType->getBitWidth() <= 32) {
            return std::make_unique<IntType>(false);
        }

        if (intType->getBitWidth() <= 64) {
            return std::make_unique<LongType>(false);
        }

        return std::make_unique<Int128>();
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
        const llvm::PointerType* PT = llvm::cast<const llvm::PointerType>(type);

        if (const llvm::FunctionType* FT = llvm::dyn_cast<llvm::FunctionType>(PT->getPointerElementType())) {
            std::vector<std::string> params;
            if (FT->getNumParams() == 0) {
                params.push_back("void");
            } else {
                std::string param;
                //parsing params to string
                for (unsigned i = 0; i < FT->getNumParams(); i++) {
                    auto paramType = getType(FT->getParamType(i));
                    param = paramType->toString();

                    if (auto PT = dynamic_cast<PointerType*>(paramType.get())) {
                        if (PT->isArrayPointer) {
                            param += " (";
                            for (unsigned i = 0; i < PT->levels; i++) {
                                param += "*";
                            }
                            param += ")" + PT->sizes;
                        }
                    }

                    if (auto AT = dynamic_cast<ArrayType*>(paramType.get())) {
                        param += AT->sizeToString();
                    }

                    params.push_back(param);
                }

                if (FT->isVarArg()) {
                    params.push_back("...");
                }
            }

            std::string paramsToString = "(";
            bool first = true;
            for (const auto& param : params) {
                if (!first) {
                    paramsToString += ", ";
                }
                first = false;

                paramsToString += param;
            }
            paramsToString += ")";

            typeDefs[type] = std::make_unique<TypeDef>(getType(FT->getReturnType())->toString() + "(*", getTypeDefName(), ")" + paramsToString);
            sortedTypeDefs.push_back(static_cast<TypeDef*>(typeDefs[type].get()));
            return typeDefs[type]->clone();
        }

        return std::make_unique<PointerType>(getType(PT->getPointerElementType()));
    }

    if (type->isStructTy()) {
        const llvm::StructType* structType = llvm::cast<const llvm::StructType>(type);

        if (!structType->hasName()) {
            program->createNewUnnamedStruct(structType);
            return std::make_unique<StructType>(program->getStruct(structType)->name);
        }

        if (structType->getName().str().compare("struct.__va_list_tag") == 0) {
            return std::make_unique<StructType>("__va_list_tag");
        }

        return std::make_unique<StructType>(getStructName(structType->getName().str()));
    }

    return nullptr;
}

std::unique_ptr<Type> TypeHandler::getBinaryType(const Type* left, const Type* right) {
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

    if (const auto UI = dynamic_cast<const Int128*>(left)) {
        return std::make_unique<Int128>();
    }
    if (const auto UI = dynamic_cast<const Int128*>(right)) {
        return std::make_unique<Int128>();
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

std::string TypeHandler::getStructName(const std::string& structName) {
    std::string name = structName;
    std::replace(name.begin(), name.end(), '.', '_');

    if (name.substr(0, 6).compare("struct") == 0) {
        name.erase(0, 7);
        name = "s_" + name;
    } else {
        //union
        name.erase(0, 6);
        name = "u_" + name;
    }

    if (name.compare("s___va_list_tag") == 0) {
        name = "__va_list_tag";
    }

    return name;
}
