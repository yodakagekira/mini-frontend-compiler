#include "ast.hpp"

void NumberExpr::accept(ASTVisitor& visitor) const {visitor.visit(*this);}
void IdentExpr::accept(ASTVisitor& visitor) const {visitor.visit(*this);}
void BinaryExpr::accept(ASTVisitor& visitor) const {visitor.visit(*this);}
void CallExpr::accept(ASTVisitor& visitor) const {visitor.visit(*this);}
void VarDeclStmt::accept(ASTVisitor& visitor) const {visitor.visit(*this);}
void AssignStmt::accept(ASTVisitor& visitor) const {visitor.visit(*this);}
void ReturnStmt::accept(ASTVisitor& visitor) const {visitor.visit(*this);}
void IfStmt::accept(ASTVisitor& visitor) const {visitor.visit(*this);}
void WhileStmt::accept(ASTVisitor& visitor) const {visitor.visit(*this);}
void BlockStmt::accept(ASTVisitor& visitor) const {visitor.visit(*this);}
void Function::accept(ASTVisitor& visitor) const {visitor.visit(*this);}
void Program::accept(ASTVisitor& visitor) const {visitor.visit(*this);}