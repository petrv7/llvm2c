#pragma once

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"

#include "Expr/Expr.h"

class Func;

/**
 * @brief The Block class represents one of the block of the LLVM function.
 */
class Block {
friend class Func;

public:
    std::string blockName;

    /**
     * @brief Block Constructor for Block.
     * @param blockName Name of the block
     * @param block llvm::BasicBlock for parsing
     * @param func Func where the block is located
     */
    Block(const std::string &blockName, const llvm::BasicBlock* block, Func* func);

    /**
     * @brief parseLLVMBlock Parses instructions of the block.
     */
    void parseLLVMBlock();

    /**
     * @brief print Prints the translated block in the llvm::outs() stream.
     */
    void print();

    /**
     * @brief saveFile Saves the translated block to the given file.
     * @param file Opened file for saving the block.
     */
    void saveFile(std::ofstream& file);

private:
    const llvm::BasicBlock* block;
    std::vector<Expr*> abstractSyntaxTree; //vector used for saving instructions of the block in form of AST, rename
    llvm::DenseMap<const llvm::Value*, std::unique_ptr<StructElement>> structElements;
    std::map<Expr*, std::unique_ptr<Expr>> derefs;
    std::map<Expr*, std::unique_ptr<Expr>> refs;
    Func* func;

    /**
     * @brief parseAllocaInstruction Parses alloca instruction into Value and RefExpr.
     * @param ins alloca instruction
     */
    void parseAllocaInstruction(const llvm::Instruction& ins);

    /**
     * @brief parseLoadInstruction Parses load instruction into DerefExpr.
     * @param ins load instruction
     */
    void parseLoadInstruction(const llvm::Instruction& ins);

    /**
     * @brief parseStoreInstruction Parses store instruction into EqualsExpr.
     * @param ins store instruction
     */
    void parseStoreInstruction(const llvm::Instruction& ins);

    /**
     * @brief parseBinaryInstruction Parses binary instruction into corresponding Expr (e.g. llvm::Instruction::Add into AddExpr)
     * @param ins binary instruction
     */
    void parseBinaryInstruction(const llvm::Instruction& ins);

    /**
     * @brief parseCmpInstruction Parses cmp instruction into CmpExpr
     * @param ins cmp instruction
     */
    void parseCmpInstruction(const llvm::Instruction& ins);

    /**
     * @brief parseBrInstruction Parses br instruction into IfExpr.
     * @param ins br instruction
     */
    void parseBrInstruction(const llvm::Instruction& ins);

    /**
     * @brief parseRetInstruction Parses ret instruction into RetExpr.
     * @param ins ret instruction
     */
    void parseRetInstruction(const llvm::Instruction& ins);

    /**
     * @brief parseSwitchInstruction Parses switch instruction into SwitchExpr.
     * @param ins switch instruction
     */
    void parseSwitchInstruction(const llvm::Instruction& ins);

    /**
     * @brief parseAsmInst Parses assembler instruction into AsmExpr.
     * @param ins unreachable instruction
     */
    void parseAsmInst(const llvm::Instruction& ins);

    /**
     * @brief parseShiftInstruction Parses shift instruction into corresponding Expr (e.g. llvm::Instruction::Shl into ShlExpr)
     * @param ins shift instruction
     */
    void parseShiftInstruction(const llvm::Instruction& ins);

    /**
     * @brief parseCallInstruction Parses call instruction into CallExpr.
     * @param ins call instruction
     */
    void parseCallInstruction(const llvm::Instruction& ins);

    /**
     * @brief parseCastInstruction Parses cast instructions into CastExpr.
     * @param ins cast instruction
     */
    void parseCastInstruction(const llvm::Instruction& ins);

    /**
     * @brief parseSelectInstruction Parses select instruction into SelectExpr.
     * @param ins select instruction
     */
    void parseSelectInstruction(const llvm::Instruction& ins);

    /**
     * @brief parseGepInstruction Parses getelementptr instruction into GepExpr.
     * @param ins getelementptr instruction
     */
    void parseGepInstruction(const llvm::Instruction& ins);

    /**
     * @brief parseLLVMInstruction Calls corresponding parse method for given instruction.
     * @param ins Instruction for parsing
     */
    void parseLLVMInstruction(const llvm::Instruction& ins);

    /**
     * @brief setMetadataInfo Sets original variable name and unsigned flag for the variable type if its metadata is found.
     * @param ins Call instruction that called llvm.dbg.declare
     */
    void setMetadataInfo(const llvm::CallInst* ins);

    /**
     * @brief unsetAllInit Resets the init flag for every variable.
     * Used for repeated calling of print and saveFile.
     */
    void unsetAllInit();

    /**
     * @brief createConstantValue Creates Value for given ConstantInt or ConstantFP and inserts it into exprMap.
     * @param val constant value
     */
    void createConstantValue(llvm::Value* val);

    /**
     * @brief parseConstantGep Parses GetElementPtrConstantExpr.
     * @param expr GetElementPtrConstantExpr for parsing
     */
    void parseConstantGep(llvm::ConstantExpr* expr) const;

    /**
     * @brief isCFunc Determines wether the LLVM function has equivalent in standard C library.
     * @param func Name of the function
     * @return True if function is standard C library function, false otherwise
     */
    bool isCFunc(const std::string& func) const;

    /**
     * @brief getCFunc Takes LLVM intrinsic function and returns name of the corresponding C function.
     * @param func LLVM intrinsic function
     * @return string containing name of the C function
     */
    std::string getCFunc(const std::string& func) const;
};
