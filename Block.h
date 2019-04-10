#pragma once

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/InlineAsm.h"

#include "Expr/Expr.h"

class Func;

/**
 * @brief The Block class represents one of the block of the LLVM function.
 */
class Block {
    friend class Func;
private:
    const llvm::BasicBlock* block;

    Func* func;

    std::vector<Expr*> abstractSyntaxTree; //vector used for saving instructions of the block in form of AST

    //getelementptr expressions
    std::vector<std::unique_ptr<GepExpr>> geps; //GepExpr vector used in getelementptr parsing
    std::vector<std::unique_ptr<Expr>> casts; //Vector of casted values used in parsing getelementptr and store instruction
    std::map<Expr*, std::unique_ptr<Expr>> refs; //Map used for storing unique pointers to RefExpr (used in parsing getelementptr instruction and constant expressions)

    //store expressions
    std::map<Expr*, std::unique_ptr<Expr>> derefs; //Map used for storing unique pointers to DerefExpr (used in store instruction parsing)

    //extractvalue expressions
    std::vector<std::unique_ptr<Value>> values; //Vector containing Values used in parsing extractvalue

    //inline asm expressions
    std::vector<std::unique_ptr<Expr>> vars; //Vector of Values used in parsing inline asm
    std::vector<std::unique_ptr<Expr>> stores; //Vector containing EqualsExpr used in parsing inline asm

    /**
     * @brief parseAllocaInstruction Parses alloca instruction into Value and RefExpr.
     * @param ins alloca instruction
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseAllocaInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief parseLoadInstruction Parses load instruction into DerefExpr.
     * @param ins load instruction
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseLoadInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief parseStoreInstruction Parses store instruction into EqualsExpr.
     * @param ins store instruction
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseStoreInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief parseBinaryInstruction Parses binary instruction into corresponding Expr (e.g. llvm::Instruction::Add into AddExpr)
     * @param ins binary instruction
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseBinaryInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief parseCmpInstruction Parses cmp instruction into CmpExpr
     * @param ins cmp instruction
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseCmpInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief parseBrInstruction Parses br instruction into IfExpr.
     * @param ins br instruction
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseBrInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief parseRetInstruction Parses ret instruction into RetExpr.
     * @param ins ret instruction
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseRetInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief parseSwitchInstruction Parses switch instruction into SwitchExpr.
     * @param ins switch instruction
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseSwitchInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief parseAsmInst Parses assembler instruction into AsmExpr.
     * @param ins unreachable instruction
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseAsmInst(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief parseShiftInstruction Parses shift instruction into corresponding Expr (e.g. llvm::Instruction::Shl into ShlExpr)
     * @param ins shift instruction
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseShiftInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief parseCallInstruction Parses call instruction into CallExpr.
     * @param ins call instruction
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseCallInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief parseCastInstruction Parses cast instructions into CastExpr.
     * @param ins cast instruction
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseCastInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief parseSelectInstruction Parses select instruction into SelectExpr.
     * @param ins select instruction
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseSelectInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief parseGepInstruction Parses getelementptr instruction into GepExpr.
     * @param ins getelementptr instruction
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseGepInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief parseExtractValueInstruction Parses extractvalue instruction into GepExpr. ?????????????????????????????????????
     * @param ins extractvalue instruction
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseExtractValueInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief parseLLVMInstruction Calls corresponding parse method for given instruction.
     * @param ins Instruction for parsing
     * @param isConstExpr indicated that ConstantExpr is being parsed
     * @param val pointer to the original ConstantExpr (ins contains ConstantExpr as instruction)
     */
    void parseLLVMInstruction(const llvm::Instruction& ins, bool isConstExpr, const llvm::Value* val);

    /**
     * @brief setMetadataInfo Uses metadata to add additional information to variables (e.g. original name, unsigness)
     * @param ins Call instruction that called llvm.dbg.declare
     */
    void setMetadataInfo(const llvm::CallInst* ins);

    /**
     * @brief createConstantValue Creates Value for given ConstantInt or ConstantFP and inserts it into exprMap.
     * @param val constant value
     */
    void createConstantValue(const llvm::Value* val);

    /**
     * @brief isVoidType Parses metadata about variable type. Returns wether the type is void or not.
     * @param type Metadata information about type
     * @return True if type is void, false otherwise.
     */
    bool isVoidType(llvm::DITypeRef type);

    /**
     * @brief createFuncCallParam Creates new Expr for parameter of function call.
     * @param param Parameter of function call
     */
    void createFuncCallParam(const llvm::Use& param);

    /**
     * @brief getAsmOutputString Parses asm constraint string to get output operands.
     * @param info ConstraintInfoVector containing parsed asm constraint string
     * @return Strings containing output operand for inline assembler
     */
    std::vector<std::string> getAsmOutputStrings(llvm::InlineAsm::ConstraintInfoVector info) const;

    /**
     * @brief getAsmInputStrings Parses asm constraint string to get input operands.
     * @param info ConstraintInfoVector containing parsed asm constraint string
     * @return Vector of strings containing input operand for inline assembler
     */
    std::vector<std::string> getAsmInputStrings(llvm::InlineAsm::ConstraintInfoVector info) const;

    /**
     * @brief getRegisterString Parses string containing register label from LLVM to C.
     * @param str LLVM register string
     * @return C register string
     */
    std::string getRegisterString(const std::string& str) const;

    /**
     * @brief getAsmUsedRegString Parses asm constraint string to get used registers.
     * @param info ConstraintInfoVector containing parsed asm constraint string
     * @return String containing used registers
     */
    std::string getAsmUsedRegString(llvm::InlineAsm::ConstraintInfoVector info) const;

    /**
     * @brief toRawString Converts string to its raw format (including escape chars etc.)
     * @param str String
     * @return String in raw format
     */
    std::string toRawString(const std::string& str) const;

    /**
     * @brief unsetAllInit Unsets the init flag in every Value in abstractSyntaxTree. Used for repeated calling of print or saveFile.
     */
    void unsetAllInit();

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

    /**
     * @brief isCFunc Determines wether the LLVM function has equivalent in standard C library.
     * @param func Name of the function
     * @return True if function is standard C library function, false otherwise
     */
    static bool isCFunc(const std::string& func);

    /**
     * @brief isCFunc Determines wether the LLVM function has equivalent in math.h
     * @param func Name of the function
     * @return True if function is in math.h, false otherwise
     */
    static bool isCMath(const std::string& func);

    /**
     * @brief getCFunc Takes LLVM intrinsic function and returns name of the corresponding C function.
     * @param func LLVM intrinsic function
     * @return string containing name of the C function
     */
    static std::string getCFunc(const std::string& func);
};
