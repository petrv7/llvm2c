#pragma once

#include <vector>

#include <llvm/Support/SourceMgr.h>
#include <llvm/IR/Module.h>
#include "llvm/ADT/DenseMap.h"

#include "Func.h"

/**
 * @brief The Program class represents the whole parsed LLVM program.
 */
class Program {
private:
    llvm::LLVMContext context;
    llvm::SMDiagnostic error;
    std::unique_ptr<llvm::Module> module;

    std::vector<std::unique_ptr<Func>> functions; // vector of parsed functions
    std::vector<std::unique_ptr<Struct>> structs; // vector of parsed structs
    llvm::DenseMap<const llvm::GlobalVariable*, std::unique_ptr<GlobalValue>> globalVars; //map containing global variables

    unsigned structVarCount;
    unsigned gvarCount;

    /**
     * @brief getVarName Creates a new name for a variable in form of string containing "var" + structVarCount.
     * @return String containing a new variable name.
     */
    std::string getStructVarName();

    /**
     * @brief getGvarName Creates a new name for a global variable in form of string containing "gvar" + gvarCount.
     * @return String containing a new variable name.
     */
    std::string getGvarName();

    /**
     * @brief getValue Return string containing value used for global variable initialization.
     * @param val llvm Constant used for initialization
     * @return Init value
     */
    std::string getValue(const llvm::Constant* val) const;

public:

    /**
     * @brief Program Constructor of a Program class, parses given file into a llvm::Module.
     * @param file Path to a file for parsing.
     */
    Program(const std::string& file);

    /**
     * @brief parseProgram Parses the whole program (structs, functions and global variables0.
     */
    void parseProgram();

    /**
     * @brief parseStructs Parses structures into Struct expression.
     */
    void parseStructs();

    /**
     * @brief parseFunctions Parses functions into corresponding expressions.
     */
    void parseFunctions();

    /**
     * @brief parseGlobalVars Parses global variables.
     */
    void parseGlobalVars();

    /**
     * @brief print Prints the translated program in the llvm::outs() stream.
     */
    void print() const;

    /**
     * @brief saveFile Saves the translated program to the file with given name.
     * @param fileName Name of the file.
     */
    void saveFile(const std::string& fileName) const;

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