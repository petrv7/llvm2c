#include "core/Program.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include <iostream>
#include <string>

using namespace llvm;

int main(int argc, char** argv) {
    cl::OptionCategory options("llvm2c options");
    cl::opt<std::string> Output("o", cl::desc("Output filename"), cl::value_desc("filename"), cl::cat(options));
    cl::opt<std::string> Input(cl::Positional, cl::Required, cl::desc("<input>"), cl::cat(options));
    cl::opt<bool> Print("p", cl::desc("Print translated program"), cl::cat(options));
    cl::opt<bool> Debug("debug", cl::desc("Print only information about translation"), cl::cat(options));
    cl::opt<bool> Includes("add-includes", cl::desc("Uses includes instead of declarations. For experimental purposes."), cl::cat(options));
    cl::opt<bool> Casts("no-function-call-casts", cl::desc("Removes casts around function calls. For experimental purposes."), cl::cat(options));

    cl::HideUnrelatedOptions(options);
    cl::ParseCommandLineOptions(argc, argv);

    if (Output.empty() && !Print && !Debug) {
        std::cout << "Output method not specified!\n";
        return 1;
    }

    try {
        Program program(Input, Includes, Casts);

        if (Print) {
            program.print();
        }

        if (!Output.empty()) {
            program.saveFile(Output);
        }
    } catch (std::invalid_argument& e) {
        std::cerr << e.what();
        return 1;
    }

    return 0;
}
