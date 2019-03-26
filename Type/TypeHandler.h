#pragma once

#include "Type.h"

#include "llvm/IR/Type.h"
#include "llvm/ADT/DenseMap.h"
#include <llvm/IR/Module.h>

#include <memory>

class Program;

class TypeHandler {
private:
    const Program* program;
    llvm::DenseMap<const llvm::StructType*, std::unique_ptr<Type>> unnamedStructs; // map containing unnamed structs

public:
    TypeHandler(const Program* program)
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
     * @brief createNewUnnamedStructType Adds new UnnamedStructType to the map unnameStructs
     * @param structPointer Pointer to the LLVM StructType
     * @param structString String containing parsed unnamed struct
     */
    void createNewUnnamedStructType(const llvm::StructType* structPointer, const std::string& structString);
};
