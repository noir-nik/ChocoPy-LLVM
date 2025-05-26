module Basic;
import :SymbolTable;
import :Diagnostic;
import LLVM;
import std;

namespace {
const char *DiagnosticText[] = {
#define DIAG(ID, LEVEL, MSG) MSG,
#include "DiagnosticKinds.def"
};

const char *getDiagnosticText(unsigned DiagId) {
  return DiagnosticText[DiagId];
}
} // namespace

namespace chocopy {
InFlightDiagnostic DiagnosticsEngine::emitError(SMLoc Loc, unsigned DiagId) {
  NumErrors++;
  return InFlightDiagnostic(this, SourceMgr::DK_Error, Loc, DiagId);
}

InFlightDiagnostic DiagnosticsEngine::emitWarning(SMLoc Loc, unsigned DiagId) {
  NumWarnings++;
  return InFlightDiagnostic(this, SourceMgr::DK_Warning, Loc, DiagId);
}

void DiagnosticsEngine::report(SourceMgr::DiagKind Kind, SMLoc Loc,
                               StringRef Msg) {
  Diagnostic Diag(Kind, Loc, Msg);
  Client->handleDiagnostic(Diag);
}

void InFlightDiagnostic::emit() {
  llvm::SmallString<100> Msg;

  switch (Args.size()) {
    break;
  case 0:
    Msg =
        formatMessage(std::make_index_sequence<0>{}, getDiagnosticText(DiagId));
    break;
  case 1:
    Msg =
        formatMessage(std::make_index_sequence<1>{}, getDiagnosticText(DiagId));
    break;
  case 2:
    Msg =
        formatMessage(std::make_index_sequence<2>{}, getDiagnosticText(DiagId));
    break;
  case 3:
    Msg =
        formatMessage(std::make_index_sequence<3>{}, getDiagnosticText(DiagId));
    break;
  case 4:
    Msg =
        formatMessage(std::make_index_sequence<4>{}, getDiagnosticText(DiagId));
    break;
  case 5:
    Msg =
        formatMessage(std::make_index_sequence<5>{}, getDiagnosticText(DiagId));
    break;
  case 6:
    Msg =
        formatMessage(std::make_index_sequence<6>{}, getDiagnosticText(DiagId));
    break;
  case 7:
    Msg =
        formatMessage(std::make_index_sequence<7>{}, getDiagnosticText(DiagId));
    break;
  case 8:
    Msg =
        formatMessage(std::make_index_sequence<8>{}, getDiagnosticText(DiagId));
    break;
  case 9:
    Msg =
        formatMessage(std::make_index_sequence<9>{}, getDiagnosticText(DiagId));
  }
  DiagEngine->report(Kind, Location, Msg);
}

void TextDiagnosticPrinter::handleDiagnostic(const Diagnostic &Diag) {
  //   auto Num = SrcMgr.getNumBuffers();
  //   std::printf("%d\n", Num);

 /*  
  auto Buf = SrcMgr.getMemoryBuffer(1);
  auto StartPtr = Buf->getBufferStart();
  auto LocPtr = Diag.getLocation().getPointer();
  auto NumLines = 1;

  // Calc lines before loc
  char const *LineStart = StartPtr;
  while (StartPtr < LocPtr) {
    if (*StartPtr == '\n') {
      LineStart = StartPtr + 1;
      NumLines++;
    }
    StartPtr++;
  }

  // Calc pos in line
  int PosInLine = LocPtr - LineStart + 1;
  //   std::printf("%d:%d: %s\n", NumLines, PosInLine,
  //   Diag.getMessage().c_str());

  auto FileName = Buf->getBufferIdentifier();
  //   std::printf("%s\n", Diag.getMessage().c_str());
  std::printf("%s:%d:%d: error: %s\n", FileName.data(), NumLines, PosInLine,
              Diag.getMessage().c_str());

  //   std::printf("%s: %s\n", FileName.data(), Diag.getMessage().c_str());
  // //   std::printf("P:%sn", Diag.getLocation().getPointer());

  SrcMgr.FindBufferContainingLoc(Diag.getLocation());
 */

	// Crash with new llvm and ubuntu
  SrcMgr.PrintMessage(Diag.getLocation(), Diag.getKind(), Diag.getMessage()); 

}

} // namespace chocopy
