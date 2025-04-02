#include "chocopy-llvm/AST/TextDiagnosticPrinter.h"

#include <llvm/Support/raw_ostream.h>

namespace chocopy {
void TextDiagnosticPrinter::handleDiagnostic(const Diagnostic &Diag) {
//   SrcMgr.PrintMessage(Diag.getLocation(), Diag.getKind(), Diag.getMessage());
  std::printf("%s\n", Diag.getMessage().c_str());

}
} // namespace chocopy