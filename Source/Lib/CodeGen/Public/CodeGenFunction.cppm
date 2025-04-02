export module CodeGen:CodeGenFunction;
import :CodeGenModule;

import Basic;
import AST;
import std;

export namespace chocopy {
namespace codegen {
class CodeGenFunction {
public:
  CodeGenFunction(CodeGenModule &CGM) : CGM(CGM), Builder(CGM.getContext()) {}
  CodeGenFunction(CodeGenModule &CGM, FuncDef *F)
      : CGM(CGM), Builder(CGM.getContext()), F(F) {}

  void emitMain();
  void emit();
  void emitDeclaration(Declaration *D);
  void emitStmt(Stmt *S);
  void emitAssignStmt(AssignStmt *A);

  llvm::Value *emitExpr(Expr *E);
  llvm::Value *emitDeclRef(DeclRef *D, bool LoadVal = true);
  llvm::Value *emitBinaryExpr(BinaryExpr *B);
  llvm::Value *emitCallExpr(CallExpr *C);
  llvm::Value *emitIntLiteral(IntegerLiteral *I);

  llvm::Value *emitCreateInitObj(DeclRef *V);

private:
  CodeGenModule &CGM;
  llvm::IRBuilder<> Builder;
  FuncDef *F = nullptr;
  llvm::Function *Fn;
  llvm::BasicBlock *BB = nullptr;
};
} // namespace codegen
} // namespace chocopy
