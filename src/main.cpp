#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "lexer.h"
#include "parser.h"
#include "ast_printer.h"
#include "interpreter.h"

int main(int argc, char* argv[]) {
    bool printAst = false;
    std::string filename;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--ast") {
            printAst = true;
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
        // Default demo program when no file is provided
        source =
            "intent \"Initialize user profile for processing\"\n"
            "let user_data be { \"name\": \"Alice\", \"age\": 30 }\n"
            "\n"
            "pipe process_user\n"
            "- start with user_data\n"
            "- map using [age]\n"
            "- filter when age greater than 18\n"
            "- yield result\n"
            "\n"
            "pipe generate_greeting\n"
            "- start with user_data\n"
            "- map using [name]\n"
            "- transform value\n"
            "- yield result\n"
            "\n"
            "match user_status\n"
            "- when \"active\" yield print \"User is active\"\n"
            "- when \"banned\" yield print \"User is banned\"\n";
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
