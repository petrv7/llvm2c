#include "Program.h"

#include "llvm/Support/raw_ostream.h"

int main(int argc, char** argv) {
    try {
        Program program(argv[1]);
        program.parseProgram();
        program.print();
        program.saveFile("stringlib.c");
    } catch (std::invalid_argument e) {
        llvm::outs() << e.what();
    }
    return 0;
}
