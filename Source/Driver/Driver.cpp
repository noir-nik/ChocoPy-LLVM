// #include "chocopy-llvm/AST/ASTContext.h"
// #include "chocopy-llvm/AST/TextDiagnosticPrinter.h"
// #include "chocopy-llvm/Analysis/CFG.h"
// #include "chocopy-llvm/CodeGen/ModuleBuilder.h"
// #include "chocopy-llvm/Lexer/Lexer.h"
// #include "chocopy-llvm/Parser/Parser.h"
// #include "chocopy-llvm/Sema/Sema.h"

import std;
import FileIOUtils;
import FileBuffer;
import Basic;
import AST;
import Lexer;
import Sema;
import LLVM;

using namespace chocopy;
using namespace llvm;

int main(int argc, char* argv[]) {
	auto file_path = "Test/Demo/demo.py";

	std::optional<std::string> content = Utils::ReadFile(file_path);
	if (!content) {
		std::printf("Failed to read file\n");
		return -1;
	}

	// MemBuffer Buffer(*content);

	auto Buffer = std::make_unique<FileBuffer>(*content);

	// std::printf("%s\n", content->c_str());

	SourceMgr SrcMgr;
	SrcMgr.AddNewSourceBuffer(std::move(Buffer), llvm::SMLoc());

	TextDiagnosticPrinter DiagPrinter(SrcMgr);
	DiagnosticsEngine     DiagsEngine(&DiagPrinter);
	Lexer TheLexer(DiagsEngine, SrcMgr);
	TheLexer.reset();
	Token TheToken;
	while (bool boolValue = TheLexer.lex(TheToken)) {
		std::printf(" %d ", boolValue);
		TheToken.print();
		std::printf("\n");
		if (TheToken.getKind() == tok::eof)
			break;
	}

	ASTContext ASTCtx(SrcMgr);
	Sema       Actions(DiagsEngine, ASTCtx);
	// Parser     TheParser(ASTCtx, TheLexer, Actions);

	// ASTCtx.initialize(TheLexer);
	// Actions.initialize();

	// if (Program* P = TheParser.parse()) {
	// 	P->dump(ASTCtx);
	// 	llvm::LLVMContext LLVMCtx;

	// 	std::unique_ptr<CodeGenerator> CodeGen = createLLVMCodegen(LLVMCtx, ASTCtx);
	// 	std::unique_ptr<llvm::Module>  M       = CodeGen->handleProgram(P, file_path);
	// }

	// chocopy::Lexer Lexer(DiagnosticsEngine(), SourceMgr);

	return 0;
}
