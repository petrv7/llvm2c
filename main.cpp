#include "core/Program.h"

#include "llvm/Support/raw_ostream.h"

#include <iostream>
#include <string>
#include <boost/program_options.hpp>

using namespace boost::program_options;

int main(int argc, char** argv) {
    options_description desc("Options");
    desc.add_options()
            ("h", "Help message")
            ("p", "Print translated program")
            ("o", value<std::string>(), "Output translated program into file specified by arg")
            ("debug", "Prints only information about translation")
            ("add-includes", "Uses includes instead of declarations. For experimental purposes.");
    variables_map vars;
    try {
        store(parse_command_line(argc, argv, desc), vars);
        notify(vars);

        if (vars.count("-h")) {
            std::cout << desc << "\n";
            return 0;
        }

        if (!vars.count("o") && !vars.count("p") && !vars.count("debug")) {
            std::cout << "Output method not specified!\n\n";
            std::cout << desc << "\n";
            return 0;
        }

        try {
            Program program(argv[1], vars.count("add-includes"));

            if (vars.count("p")) {
                program.print();
            }

            if (vars.count("o")) {
                program.saveFile(vars["o"].as<std::string>());
            }
        } catch (std::invalid_argument& e) {
            llvm::errs() << e.what();
            return 1;
        }
    } catch (error& e) {
        llvm::errs() << e.what() << "\n\n";
        std::cout << desc << "\n";
    }
    return 0;
}
