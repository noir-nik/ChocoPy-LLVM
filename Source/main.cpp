#include <llvm/IR/LLVMContext.h>
import std;
import FileIOUtils;

static llvm::LLVMContext TheContext;

int main(int argc, char* argv[]) {
	// Lexer lexer;

	auto content = Utils::ReadFile("Source/main.cpp");
	if (!content) {
		std::printf("Failed to read file\n");
		return -1;
	}

	std::printf("%s\n", content->c_str());
	
	return 0;
}
