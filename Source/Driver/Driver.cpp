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

int main(int argc, char* argv[]) {
	auto file_path = "Test/Demo/demo.py";

	std::optional<std::string> content = Utils::ReadFile(file_path);
	if (!content) {
		std::printf("Failed to read file\n");
		return -1;
	}
	// std::printf("%s\n", content->c_str());

	// MemBuffer Buffer(*content);
	auto Buffer = std::make_unique<FileBuffer>(*content);

	SourceMgr SrcMgr;
	SrcMgr.AddNewSourceBuffer(std::move(Buffer), llvm::SMLoc());

	TextDiagnosticPrinter DiagPrinter(SrcMgr);
	DiagnosticsEngine     DiagsEngine(&DiagPrinter);

	Lexer TheLexer(DiagsEngine, SrcMgr);
	TheLexer.reset();

	Token TheToken;
	while (bool BoolValue = TheLexer.lex(TheToken)) {
		TheToken.print();
		std::printf("\n");
		if (TheToken.getKind() == tok::eof)
			break;
	}

	ASTContext ASTCtx(SrcMgr);
	Sema       Actions(DiagsEngine, ASTCtx);
	Parser     TheParser(ASTCtx, TheLexer, Actions);

	ASTCtx.initialize(TheLexer.getSymbolTable());
	Actions.initialize();

	if (Program* P = TheParser.parse()) {
		P->dump(ASTCtx);
		llvm::LLVMContext LLVMCtx;

		std::unique_ptr<CodeGenerator> CodeGen = createLLVMCodegen(LLVMCtx, ASTCtx);
		std::unique_ptr<llvm::Module>  M       = CodeGen->handleProgram(P, file_path);
		// std::error_code EC;
        // llvm::raw_fd_ostream OS("a.out", EC, llvm::sys::fs::OF_Text);
        // OS << *M;
		M->print(llvm::outs(), nullptr);
	}

	return 0;
}
