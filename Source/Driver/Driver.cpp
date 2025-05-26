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

void printUsage() {
  std::printf("Usage: chocopy [options] <input_file>\n");
  std::printf("Options:\n");
  std::printf("  --ast-dump\n");
  std::printf("  --run-sema\n");
  std::printf("  --emit-llvm\n");
  std::printf("  --cfg-dump\n");
}

class FileBuffer : public llvm::MemoryBuffer {
public:
  using Base = llvm::MemoryBuffer;
  FileBuffer(StringRef Content, StringRef Identifier) : Identifier(Identifier) {
    Base::init(Content.data(), Content.data() + Content.size(), false);
  }

  virtual auto getBufferKind() const
      -> llvm::MemoryBuffer::BufferKind override {
    return MemoryBuffer_Malloc;
  }

  StringRef getBufferIdentifier() const override { return Identifier; }

  ~FileBuffer() {}

private:
  std::string Identifier;
};

int main(int Argc, char *Argv[]) {
  [[maybe_unused]] constexpr char const *DemoPath = "./test/demo/demo.py";

  std::string_view InputOpt = "-";
  StringRef OutputOpt = "-";
  //   bool DumpTokensOpt = false;
  bool AstDumpOpt = false;
  bool RunSemaOpt = false;
  bool EmitLLVMOpt = false;
  bool CfgDumpOpt = false;

  auto ArgsRange = std::span(Argv + 1, Argc - 1);

  for (std::string_view Arg : ArgsRange) {
    if (Arg == "-h" || Arg == "--help") {
      printUsage();
      return 0;
    }
  }


  char const *FirstPositional = nullptr;

  for (auto i = 1; i < Argc; ++i) {
    StringRef Arg = Argv[i];
    if (Arg == "-ast-dump") {
      AstDumpOpt = true;
    } else if (Arg == "--run-sema") {
      RunSemaOpt = true;
    } else if (Arg == "-emit-llvm") {
      EmitLLVMOpt = true;
    } else if (Arg == "-cfg-dump") {
      CfgDumpOpt = true;
    } else if (Arg == "-o") {
      if (i + 1 < Argc) {
        OutputOpt = Argv[++i];
      } else {
        std::printf("Expected output file\n");
        return -1;
      }
    } else {
      if (!FirstPositional) {
        FirstPositional = Arg.data();
      }
      if (Arg.starts_with("-")) {
        std::printf("Unknown argument: %s\n", Arg.data());
        return -1;
      }
    }
  }

  if (Argc == 1 || !FirstPositional) {
    std::printf("No input file\n");
    return -1;
  }
  InputOpt = FirstPositional;
//   std::printf("Input: %s\n", InputOpt.data());

  std::optional<std::string> Content = Utils::ReadFile(InputOpt);
  if (!Content) {
    std::printf("Failed to read file\n");
    return -1;
  }

  // std::printf("%s\n", content->c_str());

  //   std::max({Identifier.find_last_of("/\\"), Identifier.find_last_of(":")),
  //   Identifier.end()
  auto FileName = std::filesystem::path(InputOpt).filename().string();
  auto Buffer = std::make_unique<FileBuffer>(*Content, FileName);

  SourceMgr SrcMgr;
  SrcMgr.AddNewSourceBuffer(std::move(Buffer), llvm::SMLoc());

  TextDiagnosticPrinter DiagPrinter(SrcMgr);
  DiagnosticsEngine DiagsEngine(&DiagPrinter);

  Lexer TheLexer(DiagsEngine, SrcMgr);
  TheLexer.reset();

  
  // TheLexer.reset();

  ASTContext ASTCtx(SrcMgr);
  Sema Actions(DiagsEngine, ASTCtx);
  Parser TheParser(ASTCtx, TheLexer, Actions);

  ASTCtx.initialize(TheLexer.getSymbolTable());
  Actions.initialize();

  if (Program *P = TheParser.parse()) {
    if (AstDumpOpt) {
      P->dump(ASTCtx);
      std::printf("\n");
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
//   if (int ErrCnt = DiagsEngine.getNumErrors())
//     llvm::outs() << ErrCnt << " error" << (ErrCnt == 1 ? "" : "s")
//                  << " generated!" << "\n";

  auto ErrNum = DiagsEngine.getNumErrors();
  if (ErrNum > 0) {
    std::fprintf(stderr, "%u error%s generated!\n", ErrNum,
                 ErrNum == 1 ? "" : "s");
  }

  return 0;
}
