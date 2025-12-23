#pragma once

#include "../ast/ast.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace codegen {

// Minimal LLVM code generator for your toy language.
// - int only (i32)
// - locals via alloca/load/store
// - supports: var decl, assign, return, if/else, while, blocks, function defs + calls,
//   arithmetic (+ - * /)
//
// Typical usage:
//   codegen::LLVMCodeGen cg("my_module");
//   if (!cg.generate(*program)) { std::cerr << cg.error() << "\n"; }
//   cg.writeIRToFile("out.ll");
//
class LLVMCodeGen final : public ASTVisitor {
public:
    explicit LLVMCodeGen(std::string moduleName = "module");

    bool generate(const Program& prog);             // AST -> LLVM IR (module)
    bool writeIRToFile(const std::string& path) const;

    llvm::Module& module() { return *mod; }
    llvm::LLVMContext& context() { return ctx; }

    bool hadError() const { return hasErr; }
    const std::string& error() const { return errMsg; }

    // ---- ASTVisitor overrides ----
    void visit(const NumberExpr& expr) override;
    void visit(const IdentExpr& expr) override;
    void visit(const BinaryExpr& expr) override;
    void visit(const CallExpr& expr) override;

    void visit(const VarDeclStmt& stmt) override;
    void visit(const AssignStmt& stmt) override;
    void visit(const ReturnStmt& stmt) override;
    void visit(const IfStmt& stmt) override;
    void visit(const WhileStmt& stmt) override;
    void visit(const BlockStmt& stmt) override;

    void visit(const Function& func) override;
    void visit(const Program& prog) override;

private:
    void setError(const std::string& msg);

    llvm::Type* i32() const;
    llvm::Value* toBool(llvm::Value* v);

    llvm::AllocaInst* createEntryAlloca(llvm::Function* fn, const std::string& name);

    void pushScope();
    void popScope();
    llvm::AllocaInst* lookupLocal(const std::string& name) const;
    bool declareLocal(const std::string& name, llvm::AllocaInst* slot);

    void predeclareFunctions(const Program& prog);

private:
    llvm::LLVMContext ctx;
    std::unique_ptr<llvm::Module> mod;
    llvm::IRBuilder<> builder;

    // Expression result slot
    llvm::Value* lastValue = nullptr;

    // Scopes (stack of local variable maps)
    struct Scope {
    std::unordered_map<std::string, llvm::AllocaInst*> locals;
    };

    std::vector<Scope> scopes;

    // Errors
    bool hasErr = false;
    std::string errMsg;
};

} 