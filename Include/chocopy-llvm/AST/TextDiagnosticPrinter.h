#ifndef CHOCOPY_LLVM_AST_TEXT_DIAGNOSTICPRINTER_H
#define CHOCOPY_LLVM_AST_TEXT_DIAGNOSTICPRINTER_H

#include "chocopy-llvm/Basic/Diagnostic.h"
#include "chocopy-llvm/Basic/LLVM.h"

namespace chocopy {
class TextDiagnosticPrinter final : public DiagnosticConsumer {
public:
	//   TextDiagnosticPrinter(SourceMgr &SrcMgr)
	//       : DiagnosticConsumer(), SrcMgr(SrcMgr) {}

	void handleDiagnostic(const Diagnostic& Diag);/*  {
		// SrcMgr.PrintMessage(Diag.getLocation(), Diag.getKind(), Diag.getMessage());
		std::printf("%s\n", Diag.getMessage().c_str());
	}; */

private:
	//   SourceMgr &SrcMgr;
};
} // namespace chocopy

#endif // CHOCOPY_LLVM_AST_TEXT_DIAGNOSTICPRINTER_H
