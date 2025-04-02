module CodeGen;
import Basic;
import AST;
import std;
import :ModuleBuilder;

namespace chocopy {
std::unique_ptr<llvm::Module>
CodeGenerator::handleProgram(Program *Program, std::string FileName) {
  Module.reset(new llvm::Module(FileName, Ctx));
  Builder.reset(new codegen::CodeGenModule(ASTCtx, *Module));
  Builder->release();

  // for (Declaration *D : Program->getDeclarations())
//   for (Declaration *D : Program->getDeclarations()) {
// 	if (D->getKind() == DeclKind::Function) {
// 	  Builder->emitFunction(cast<FunctionDecl>(D));
// 	}
//   }
  return std::move(Module);
}

std::unique_ptr<CodeGenerator> createLLVMCodegen(llvm::LLVMContext &Ctx,
                                                 ASTContext &ASTCtx) {
  return std::make_unique<CodeGenerator>(Ctx, ASTCtx);
}
} // namespace chocopy