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
friend class Program;
private:
    std::unique_ptr<Type> returnType;

    const llvm::Function* function;
    Program* program;

    llvm::DenseMap<const llvm::BasicBlock*, std::unique_ptr<Block>> blockMap; //DenseMap used for mapping llvm::BasicBlock to Block
    llvm::DenseMap<const llvm::Value*, std::unique_ptr<Expr>> exprMap; // DenseMap used for mapping llvm::Value to Expr

    unsigned varCount; //counter for assigning names of variables
    unsigned blockCount; // counter for assigning names of blocks
    bool isDeclaration; //function is only being declared
    bool isVarArg; //function has variable number of arguments

    Expr* lastArg; //last argument before variable arguments

    /**
     * @brief getBlockName Returns name of the block if the block already has an assigned name.
     * Otherwise assigns new name for the block in form of string containing block + blockCount,
     * inserts the block to the blockMap and return the assigned name.
     * @param block llvm::BasicBlock
     * @return Name assigned for given block.
     */
    std::string getBlockName(const llvm::BasicBlock* block); //RENAME

    /**
     * @brief getExpr Finds Expr in exprMap or globalRefs with key val. If val is function, creates Value containing refference to the function and returns pointer to this Value.
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
     * @brief getVarName Creates a new name for a variable in form of string containing "var" + varCount.
     * @return String containing a variable name.
     */
    std::string getVarName();

    /**
     * @brief createNewUnnamedStruct
     * @param strct
     */
    void createNewUnnamedStruct(const llvm::StructType* strct);

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
    void print();

    /**
     * @brief saveFile Saves the translated function to the given file.
     * @param file Opened file for saving the function.
     */
    void saveFile(std::ofstream& file);

    /**
     * @brief getStruct Returns pointer to the Struct corresponding to the given LLVM StructType.
     * @param strct LLVM StructType
     * @return Pointer to Struct expression if the struct is found, nullptr otherwise
     */
    Struct* getStruct(const llvm::StructType* strct) const;

    /**
     * @brief getStruct Returns pointer to the Struct with the given name.
     * @param name Name of the struct
     * @return Pointer to Struct expression if the struct is found, nullptr otherwise
     */
    Struct* getStruct(const std::string& name) const;

    /**
     * @brief getGlobalVar Returns corresponding refference to GlobalValue expression.
     * @param val llvm global variable
     * @return RefExpr expression
     */
    RefExpr* getGlobalVar(llvm::Value* val) const;

    /**
     * @brief addDeclaration Adds new declaration of given function.
     * @param func LLVM Function
     */
    void addDeclaration(llvm::Function* func);

    /**
     * @brief stackIgnored Indicated that intrinsic stacksave/stackrestore was ignored.
     */
    void stackIgnored();

    /**
     * @brief hasInf Indicates that program uses "math.h".
     */
    void hasMath();

    /**
     * @brief getType Transforms llvm::Type into corresponding Type object
     * @param type llvm::Type for transformation
     * @return unique_ptr to corresponding Type object
     */
    std::unique_ptr<Type> getType(const llvm::Type* type, bool voidType = false);

    /**
     * @brief changeExprKey Changes key of expr in exprMap to val.
     * @param expr Original expression
     * @param val New LLVM Value
     */
    void changeExprKey(Expr* expr, const llvm::Value* val);
};
