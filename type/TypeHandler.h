#pragma once

#include "Type.h"
#include "../expr/Expr.h"

#include "llvm/IR/Type.h"
#include "llvm/ADT/DenseMap.h"
#include <llvm/IR/Module.h>

#include <memory>

class Program;

class TypeHandler {
private:
    Program* program;
    llvm::DenseMap<const llvm::Type*, std::unique_ptr<Type>> typeDefs;

    unsigned typeDefCount = 0;

    std::string getTypeDefName() {
        std::string ret = "typeDef_" + std::to_string(typeDefCount);
        typeDefCount++;
        return ret;
    }

public:
    std::vector<const TypeDef*> sortedTypeDefs;

    TypeHandler(Program* program)
        : program(program) { }

    /**
     * @brief getType Transforms llvm::Type into corresponding Type object
     * @param type llvm::Type for transformation
     * @return unique_ptr to corresponding Type object
     */
    std::unique_ptr<Type> getType(const llvm::Type* type, bool voidType = false);

    /**
     * @brief getBinaryType Returns type that would be result of a binary operation
     * @param left left argument of the operation
     * @param right right argument of the operation
     * @return unique_ptr to Type object
     */
    static std::unique_ptr<Type> getBinaryType(const Type* left, const Type* right);

    /**
     * @brief getStructName Parses LLVM struct (union) name into llvm2c struct name.
     * @param structName LLVM struct name
     * @return New struct name
     */
    static std::string getStructName(const std::string& structName);

    bool hasTypeDefs() const {
        return !typeDefs.empty();
    }
};
