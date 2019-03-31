#define private public

#include "catch.hpp"

#include "../Program.h"

TEST_CASE("Basic Program class tests", "[program]") {
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module = std::make_unique<llvm::Module>("module", context);
    Program program(module);
    llvm::outs().flush();

    REQUIRE(program.functions.empty());
}
