#include "Program.h"

#include "llvm/Support/raw_ostream.h"

int main(int argc, char** argv) {
    try {
        Program program(argv[1]);
        program.print();
        program.saveFile(program.fileName + ".c");
    } catch (std::invalid_argument e) {
        llvm::outs() << e.what();
    }
    return 0;
}
