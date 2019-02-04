#pragma once

#include <vector>

#include <llvm/Support/SourceMgr.h>
#include <llvm/IR/Module.h>

#include "Func.h"

class Program {
private:
    llvm::LLVMContext context;
    llvm::SMDiagnostic error;
    std::unique_ptr<llvm::Module> module;

    std::vector<std::unique_ptr<Func>> functions; // vector of parsed functions
    std::vector<std::unique_ptr<Struct>> structs; // vector of parsed structs

    unsigned structVarCount;

    /**
     * @brief getVarName Creates a new name for a variable in form of string containing "var" + varCount.
     * @return String containing a variable name.
     */
    std::string getStructVarName();

public:

    /**
     * @brief Program Constructor of a Program class, parses given file into a llvm::Module.
     * @param file Path to a file for parsing.
     */
    Program(const std::string& file);

    /**
     * @brief parseProgram
     */
    void parseProgram();

    /**
     * @brief print Prints the translated program in the llvm::outs() stream.
     */
    void print() const;

    /**
     * @brief saveFile Saves the translated program to the file with given name.
     * @param fileName Name of the file.
     */
    void saveFile(const std::string& fileName) const;

    Struct* getStruct(const std::string& name) const;
};
