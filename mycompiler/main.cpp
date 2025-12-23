#include <fstream>
#include <iostream>
#include <sstream>
#include <string>


#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "sema/sema.hpp"
#include "llvm/llvm_codegen.hpp"

static std::string readFileOrDie(const std::string& path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        std::cerr << "error: cannot open file: " << path << "\n";
        std::exit(1);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static void usage(const char* argv0) {
    std::cerr
        << "Usage: " << argv0 << " <input.tiny> [out.ll]\n"
        << "  Compiles a tiny C-like subset to LLVM IR.\n"
        << "  If out.ll is omitted, defaults to ./out.ll\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        usage(argv[0]);
        return 2;
    }

    const std::string inputPath = argv[1];
    const std::string outPath   = (argc >= 3) ? argv[2] : "out.ll";

    // 1) Read file
    std::string source = readFileOrDie(inputPath);

    // 2) Parse -> AST
    Lexer lexer(source);
    Parser parser(lexer);

    auto program = parser.parseProgram();
    if (!program || parser.hasError()) {
        std::cerr << "error: parse failed\n";
        return 1;
    }

    // 3) Semantic analysis
    Sema sema;
    sema.analyze(*program);
    if (sema.hadError()) {
        std::cerr << "error: semantic analysis failed\n";
        return 1;
    }

    // 4) Codegen -> LLVM IR
    codegen::LLVMCodeGen cg("mycompiler");
    if (!cg.generate(*program)) {
        std::cerr << "error: codegen failed: " << cg.error() << "\n";
        return 1;
    }

    if (!cg.writeIRToFile(outPath)) {
        std::cerr << "error: failed to write IR to: " << outPath << "\n";
        return 1;
    }

    std::cout << "Wrote LLVM IR to " << outPath << "\n";
    return 0;
}