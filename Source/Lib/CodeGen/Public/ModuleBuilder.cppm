export module CodeGen:ModuleBuilder;
import :CodeGenModule;
import Basic;
import AST;
import std;

export namespace chocopy {
class CodeGenerator {
public:
  CodeGenerator(llvm::LLVMContext &Ctx, ASTContext &ASTCtx)
      : Ctx(Ctx), ASTCtx(ASTCtx) {}

  std::unique_ptr<llvm::Module> handleProgram(Program *Program,
                                              std::string FileName);

private:
  llvm::LLVMContext &Ctx;
  ASTContext &ASTCtx;

  std::unique_ptr<llvm::Module> Module;
  std::unique_ptr<codegen::CodeGenModule> Builder;
};

std::unique_ptr<CodeGenerator> createLLVMCodegen(llvm::LLVMContext &Ctx,
                                                 ASTContext &ASTCtx);
} // namespace chocopy
