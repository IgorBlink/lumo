#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "lexer.h"
#include "parser.h"
#include "ast_printer.h"
#include "interpreter.h"

int main(int argc, char* argv[]) {
    bool printAst  = false;
    bool validate  = false;
    std::string filename;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--ast") {
            printAst = true;
        } else if (arg == "--validate") {
            validate = true;
        } else {
            filename = arg;
        }
    }

    std::string source;

    if (!filename.empty()) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Could not open file: " << filename << std::endl;
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        source = buffer.str();
    } else {
        // Read from stdin when no file is provided
        std::stringstream buffer;
        buffer << std::cin.rdbuf();
        source = buffer.str();
        if (source.empty()) {
            std::cerr << "Usage: lumo [--ast] [--validate] <file.lumo>" << std::endl;
            std::cerr << "       lumo [--ast] [--validate]   (reads from stdin)" << std::endl;
            return 1;
        }
    }

    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto program = parser.parse();

    if (printAst) {
        ASTPrinter printer;
        program->accept(printer);
        return 0;
    }

    // Change 8: --validate runs parse only and reports result without executing
    if (validate) {
        if (parser.hasErrors()) {
            std::cerr << "INVALID" << std::endl;
            return 1;
        }
        std::cout << "OK" << std::endl;
        return 0;
    }

    try {
        Interpreter interpreter;
        interpreter.execute(*program);
    } catch (const HaltException& e) {
        std::cerr << "Program halted: " << e.message << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
