#pragma once

#include <vector>
#include <map>

#include "llvm/ADT/DenseMap.h"

class Program;

#include "Expr/Expr.h"
#include "Expr/UnaryExpr.h"
#include "Expr/BinaryExpr.h"
#include "Block.h"
#include "Program.h"

/**
 * @brief The Func class represents one of the functions of the LLVM program.
 */
class Func {
friend class Block;
private:
    std::unique_ptr<Type> returnType;

    const llvm::Function* function;
    const Program* program;

    llvm::DenseMap<const llvm::BasicBlock*, std::unique_ptr<Block>> blockMap; //DenseMap used for mapping llvm::BasicBlock to Block
    llvm::DenseMap<const llvm::Value*, std::unique_ptr<Expr>> exprMap; // DenseMap used for mapping llvm::Value to Expr
    llvm::DenseMap<const llvm::Value*, std::unique_ptr<Value>> valueMap; //DenseMap used in parsing alloca instruction for mapping llvm::Value to Value
    llvm::DenseMap<const llvm::Value*, std::unique_ptr<GepExpr>> gepExprMap; //DenseMap used in parsing getelementptr instruction for mapping llvm::Value to GepExpr
    llvm::DenseMap<const llvm::Value*, std::unique_ptr<CallExpr>> callExprMap; //DenseMap used in parsing call instruction for mapping llvm::Value to CallExpr
    llvm::DenseMap<const llvm::Value*, std::unique_ptr<EqualsExpr>> callValueMap; //DenseMap used in parsing call instruction for mapping llvm::Value to EqualsExpr

    unsigned varCount; //counter for assigning names of variables
    unsigned blockCount; // counter for assigning names of blocks
    bool isDeclaration;
    bool isVarArg;

    /**
     * @brief getBlockName Returns name of the block if the block already has an assigned name.
     * Otherwise assigns new name for the block in form of string containing block + blockCount,
     * inserts the block to the blockMap and return the assigned name.
     * @param block llvm::BasicBlock
     * @return Name assigned for given block.
     */
    std::string getBlockName(const llvm::BasicBlock* block); //RENAME

    /**
     * @brief getExpr Finds Expr in exprMap with key val.
     * @param val Key of the Expr
     * @return Pointer to the Expr if val is found, nullptr otherwise.
     */
    Expr* getExpr(const llvm::Value* val);

    /**
     * @brief createExpr Inserts expr into the exprMap using val as a key.
     * @param val Key
     * @param expr Mapped Value
     */
    void createExpr(const llvm::Value* val, std::unique_ptr<Expr> expr);

    /**
     * @brief createExpr Casts ins to const llvm::Value* and calls createExpr
     * @param ins llvm::Instruction
     * @param expr Mapped Value
     */
    void createExpr(const llvm::Instruction* ins, std::unique_ptr<Expr> expr);

    /**
     * @brief getVarName Creates a new name for a variable in form of string containing "var" + varCount.
     * @return String containing a variable name.
     */
    std::string getVarName();

public:

    /**
     * @brief Func Constructor for Func.
     * @param func llvm::Function for parsing
     * @param program Program to which function belongs
     * @param isDeclaration bool signalizing that function is only being declared
     */
    Func(llvm::Function* func, Program* program, bool isDeclaration);

    /**
     * @brief parseFunction Parses blocks of the llvm::Function.
     */
    void parseFunction();

    /**
     * @brief print Prints the translated function in the llvm::outs() stream.
     */
    void print() const;

    /**
     * @brief saveFile Saves the translated function to the given file.
     * @param file Opened file for saving the function.
     */
    void saveFile(std::ofstream& file) const;

    /**
     * @brief getStruct Returns Struct expression with the given name.
     * @param name Name of the struct
     * @return Struct expression if the struct is found, nullptr otherwise
     */
    Struct* getStruct(const std::string& name) const;

    /**
     * @brief getGlobalVar Returns corresponding GlobalValue expression.
     * @param val llvm global variable
     * @return GlobalValue expression
     */
    GlobalValue* getGlobalVar(llvm::Value* val) const;
};
