#include <cstdio>

import std;
import FileIOUtils;
import FileBuffer;
import Basic;
import AST;
import Lexer;
import Sema;
import Parser;
import CodeGen;
import LLVM;

using namespace chocopy;
using namespace llvm;

void dumpTokens(Lexer &TheLexer) {
  Token TheToken;
  while (bool BoolValue = TheLexer.lex(TheToken)) {
    TheToken.print();
    std::printf("\n");
    if (TheToken.getKind() == tok::eof)
      break;
  }
}

int main(int Argc, char *Argv[]) {
  constexpr char const *DemoPath = "./Test/Demo/demo.py";

  bool DumpTokensOpt = false;
  bool DumpASTOpt = false;
  bool RunSemaOpt = false;
  bool EmitLLVMOpt = false;
  bool CfgDumpOpt = false;

  for (std::string_view Arg : std::span(Argv + 1, Argc - 1)) {
    if (Arg == "-t") {
      DumpTokensOpt = true;
    } else if (Arg == "-ast-dump") {
      DumpASTOpt = true;
    } else if (Arg == "-run-sema") {
      RunSemaOpt = true;
    } else if (Arg == "-emit-llvm") {
      EmitLLVMOpt = true;
    } else if (Arg == "-cfg-dump") {
      CfgDumpOpt = true;
    }
  }

  if (Argc == 1) {
    std::printf("No input file\n");
    return -1;
  }
  char const *FilePath = Argv[1];

  std::optional<std::string> Content = Utils::ReadFile(FilePath);
  if (!Content) {
    std::printf("Failed to read file\n");
    return -1;
  }

  // std::printf("%s\n", content->c_str());

  auto Buffer = std::make_unique<FileBuffer>(*Content);

  SourceMgr SrcMgr;
  SrcMgr.AddNewSourceBuffer(std::move(Buffer), llvm::SMLoc());

  TextDiagnosticPrinter DiagPrinter(SrcMgr);
  DiagnosticsEngine DiagsEngine(&DiagPrinter);

  Lexer TheLexer(DiagsEngine, SrcMgr);
  TheLexer.reset();

  if (DumpTokensOpt) {
    dumpTokens(TheLexer);
    return 0;
  }
  // TheLexer.reset();

  ASTContext ASTCtx(SrcMgr);
  Sema Actions(DiagsEngine, ASTCtx);
  Parser TheParser(ASTCtx, TheLexer, Actions);

  ASTCtx.initialize(TheLexer.getSymbolTable());
  Actions.initialize();

  if (Program *P = TheParser.parse()) {
    auto ErrNum = DiagsEngine.getNumErrors();
    if (ErrNum > 0) {
      std::fprintf(stderr, "%u error%s generated!\n", ErrNum,
                   ErrNum == 1 ? "" : "s");
    }

    if (DumpASTOpt) {
      P->dump(ASTCtx);
    }

    if (RunSemaOpt || EmitLLVMOpt)
      Actions.run();

    // llvm::LLVMContext LLVMCtx;

    // std::unique_ptr<CodeGenerator> CodeGen = createLLVMCodegen(LLVMCtx,
    // ASTCtx); std::unique_ptr<llvm::Module>  M       =
    // CodeGen->handleProgram(P, file_path);
    // // std::error_code EC;
    // // llvm::raw_fd_ostream OS("a.out", EC, llvm::sys::fs::OF_Text);
    // // OS << *M;
    // M->print(llvm::outs(), nullptr);
  }

  return 0;
}
