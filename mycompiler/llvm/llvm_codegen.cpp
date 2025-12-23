#include "llvm_codegen.hpp"

#include <unordered_map>
#include <vector>

#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>

namespace codegen {

LLVMCodeGen::LLVMCodeGen(std::string moduleName)
    : mod(std::make_unique<llvm::Module>(moduleName, ctx)),
      builder(ctx) {}

void LLVMCodeGen::setError(const std::string& msg) {
    if (!hasErr) {
        hasErr = true;
        errMsg = msg;
    }
}

llvm::Type* LLVMCodeGen::i32() const {
    return llvm::Type::getInt32Ty(const_cast<llvm::LLVMContext&>(ctx));
}

llvm::Value* LLVMCodeGen::toBool(llvm::Value* v) {
    // int != 0
    auto* zero = llvm::ConstantInt::get(i32(), 0, true);
    return builder.CreateICmpNE(v, zero, "cond");
}

llvm::AllocaInst* LLVMCodeGen::createEntryAlloca(llvm::Function* fn, const std::string& name) {
    llvm::IRBuilder<> tmp(&fn->getEntryBlock(), fn->getEntryBlock().begin());
    return tmp.CreateAlloca(i32(), nullptr, name);
}

void LLVMCodeGen::pushScope() { scopes.push_back(Scope{}); }

void LLVMCodeGen::popScope()  { if (!scopes.empty()) scopes.pop_back(); }

llvm::AllocaInst* LLVMCodeGen::lookupLocal(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto f = it->locals.find(name);
        if (f != it->locals.end()) return f->second;
    }
    return nullptr;
}

bool LLVMCodeGen::declareLocal(const std::string& name, llvm::AllocaInst* slot) {
    if (scopes.empty()) pushScope();
    auto& top = scopes.back().locals;
    if (top.count(name)) return false;
    top[name] = slot;
    return true;
}

void LLVMCodeGen::predeclareFunctions(const Program& prog) {
    for (const auto& fptr : prog.getFunctions()) {
        const Function& fnAst = *fptr;
        const std::string& name = fnAst.getName();

        // All params are int
        std::vector<llvm::Type*> argTypes(fnAst.getParams().size(), llvm::Type::getInt32Ty(ctx));
        auto* fty = llvm::FunctionType::get(llvm::Type::getInt32Ty(ctx), argTypes, false);

        if (mod->getFunction(name)) continue; // sema should catch duplicates
        auto* fn = llvm::Function::Create(fty, llvm::Function::ExternalLinkage, name, mod.get());

        // Name parameters (nice IR)
        size_t i = 0;
        for (auto& arg : fn->args()) {
            arg.setName(fnAst.getParams()[i++]);
        }
    }
}

bool LLVMCodeGen::generate(const Program& prog) {
    hasErr = false;
    errMsg.clear();
    lastValue = nullptr;
    scopes.clear();

    // Make function prototypes first so forward calls work
    predeclareFunctions(prog);

    // Then generate bodies
    prog.accept(*this);

    if (hasErr) return false;

    // Verify (optional but helpful; still "minimal")
    std::string verifyMsg;
    llvm::raw_string_ostream os(verifyMsg);
    if (llvm::verifyModule(*mod, &os)) {
        os.flush();
        setError("LLVM verify failed:\n" + verifyMsg);
        return false;
    }

    return true;
}

bool LLVMCodeGen::writeIRToFile(const std::string& path) const {
    std::error_code ec;
    llvm::raw_fd_ostream out(path, ec, llvm::sys::fs::OF_Text);
    if (ec) return false;
    mod->print(out, nullptr);
    return true;
}

// -------------------- Visitor: Program / Function --------------------

void LLVMCodeGen::visit(const Program& prog) {
    for (const auto& f : prog.getFunctions()) {
        f->accept(*this);
        if (hasErr) return;
    }
}

void LLVMCodeGen::visit(const Function& func) {
    llvm::Function* fn = mod->getFunction(func.getName());
    if (!fn) {
        setError("CodeGen internal: missing prototype for function '" + func.getName() + "'");
        return;
    }
    if (!fn->empty()) {
        setError("Duplicate function definition for '" + func.getName() + "'");
        return;
    }

    scopes.clear();
    pushScope();

    auto* entry = llvm::BasicBlock::Create(ctx, "entry", fn);
    builder.SetInsertPoint(entry);

    // Lower params to allocas
    for (auto& arg : fn->args()) {
        auto name = std::string(arg.getName());
        auto* slot = createEntryAlloca(fn, name);
        builder.CreateStore(&arg, slot);
        declareLocal(name, slot);
    }

    // Body
    func.getBody()->accept(*this);
    if (hasErr) return;

    // Default return if none emitted
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateRet(llvm::ConstantInt::get(i32(), 0, true));
    }

    popScope();
}

// -------------------- Visitor: Statements --------------------

void LLVMCodeGen::visit(const BlockStmt& stmt) {
    pushScope();
    for (const auto& s : stmt.getStmts()) {
        if (builder.GetInsertBlock()->getTerminator()) break;
        s->accept(*this);
        if (hasErr) break;
    }
    popScope();
}

void LLVMCodeGen::visit(const VarDeclStmt& stmt) {
    auto* fn = builder.GetInsertBlock()->getParent();
    auto* slot = createEntryAlloca(fn, stmt.getName());

    if (!declareLocal(stmt.getName(), slot)) {
        setError("Duplicate variable in same scope: '" + stmt.getName() + "'");
        return;
    }

    llvm::Value* initV = llvm::ConstantInt::get(i32(), 0, true);
    if (stmt.getInit()) {
        stmt.getInit()->accept(*this);
        if (hasErr) return;
        initV = lastValue;
    }
    builder.CreateStore(initV, slot);
}

void LLVMCodeGen::visit(const AssignStmt& stmt) {
    auto* slot = lookupLocal(stmt.getName());
    if (!slot) {
        setError("Assignment to undefined variable: '" + stmt.getName() + "'");
        return;
    }
    stmt.getValue()->accept(*this);
    if (hasErr) return;
    builder.CreateStore(lastValue, slot);
}

void LLVMCodeGen::visit(const ReturnStmt& stmt) {
    stmt.getExpr()->accept(*this);
    if (hasErr) return;
    builder.CreateRet(lastValue);
}

void LLVMCodeGen::visit(const IfStmt& stmt) {
    auto* curFn = builder.GetInsertBlock()->getParent();

    // cond
    stmt.getCond()->accept(*this);
    if (hasErr) return;
    llvm::Value* cond = toBool(lastValue);

    auto* thenBB  = llvm::BasicBlock::Create(ctx, "then", curFn);
    auto* mergeBB = llvm::BasicBlock::Create(ctx, "ifend", curFn);

    llvm::BasicBlock* elseBB = nullptr;
    if (stmt.getElse()) {
        elseBB = llvm::BasicBlock::Create(ctx, "else", curFn);
        builder.CreateCondBr(cond, thenBB, elseBB);
    } else {
        builder.CreateCondBr(cond, thenBB, mergeBB);
    }

    // then
    builder.SetInsertPoint(thenBB);
    stmt.getThen()->accept(*this);
    if (hasErr) return;
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(mergeBB);
    }

    // else
    if (stmt.getElse()) {
        builder.SetInsertPoint(elseBB);
        stmt.getElse()->accept(*this);
        if (hasErr) return;
        if (!builder.GetInsertBlock()->getTerminator()) {
            builder.CreateBr(mergeBB);
        }
    }

    builder.SetInsertPoint(mergeBB);
}

void LLVMCodeGen::visit(const WhileStmt& stmt) {
    auto* curFn = builder.GetInsertBlock()->getParent();

    auto* condBB = llvm::BasicBlock::Create(ctx, "while.cond", curFn);
    auto* bodyBB = llvm::BasicBlock::Create(ctx, "while.body", curFn);
    auto* endBB  = llvm::BasicBlock::Create(ctx, "while.end",  curFn);

    builder.CreateBr(condBB);

    // cond
    builder.SetInsertPoint(condBB);
    stmt.getCond()->accept(*this);
    if (hasErr) return;
    llvm::Value* cond = toBool(lastValue);
    builder.CreateCondBr(cond, bodyBB, endBB);

    // body
    builder.SetInsertPoint(bodyBB);
    stmt.getBody()->accept(*this);
    if (hasErr) return;
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(condBB);
    }

    builder.SetInsertPoint(endBB);
}

// -------------------- Visitor: Expressions --------------------

void LLVMCodeGen::visit(const NumberExpr& expr) {
    lastValue = llvm::ConstantInt::get(i32(), expr.getValue(), true);
}

void LLVMCodeGen::visit(const IdentExpr& expr) {
    auto* slot = lookupLocal(expr.getName());
    if (!slot) {
        setError("Use of undefined variable: '" + expr.getName() + "'");
        lastValue = nullptr;
        return;
    }
    lastValue = builder.CreateLoad(i32(), slot, expr.getName() + ".val");
}

void LLVMCodeGen::visit(const BinaryExpr& expr) {
    expr.getLHS()->accept(*this);
    if (hasErr) return;
    auto* lhs = lastValue;

    expr.getRHS()->accept(*this);
    if (hasErr) return;
    auto* rhs = lastValue;

    switch (expr.getOp()) {
        case TokenType::plus:  lastValue = builder.CreateAdd(lhs, rhs, "addtmp"); break;
        case TokenType::minus: lastValue = builder.CreateSub(lhs, rhs, "subtmp"); break;
        case TokenType::star:  lastValue = builder.CreateMul(lhs, rhs, "multmp"); break;
        case TokenType::slash: lastValue = builder.CreateSDiv(lhs, rhs, "divtmp"); break;
        default:
            setError("Unsupported binary operator in codegen");
            lastValue = nullptr;
            break;
    }
}

void LLVMCodeGen::visit(const CallExpr& expr) {
    llvm::Function* callee = mod->getFunction(expr.getCallee());
    if (!callee) {
        setError("Call to unknown function: '" + expr.getCallee() + "'");
        lastValue = nullptr;
        return;
    }

    if (callee->arg_size() != expr.getArgs().size()) {
        setError("Wrong number of args in call to '" + expr.getCallee() + "'");
        lastValue = nullptr;
        return;
    }

    std::vector<llvm::Value*> args;
    args.reserve(expr.getArgs().size());
    for (const auto& a : expr.getArgs()) {
        a->accept(*this);
        if (hasErr) return;
        args.push_back(lastValue);
    }

    lastValue = builder.CreateCall(callee, args, expr.getCallee() + ".call");
}

} 