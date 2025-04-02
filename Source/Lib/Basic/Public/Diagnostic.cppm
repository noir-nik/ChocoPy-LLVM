export module Basic:Diagnostic;
import std;
import LLVM;

export namespace chocopy {
using llvm::SmallVector;
using llvm::SMLoc;
using llvm::SourceMgr;
using llvm::StringRef;

class DiagnosticsEngine;
class InFlightDiagnostic;

namespace diag {
enum {
#define DIAG(ID, LEVEL, MSG) ID,
#include "DiagnosticKinds.def"
};
} // namespace diag

class Diagnostic {
public:
  Diagnostic(SourceMgr::DiagKind Kind, SMLoc Loc, const llvm::Twine &Msg)
      : Location(Loc), Kind(Kind) {
    llvm::SmallVector<char> Str;
    Message = Msg.toNullTerminatedStringRef(Str);
  }

  const std::string &getMessage() const { return Message; }
  SMLoc getLocation() const { return Location; }
  SourceMgr::DiagKind getKind() const { return Kind; }

private:
  std::string Message;
  SMLoc Location;
  SourceMgr::DiagKind Kind;
};

class DiagnosticConsumer {
public:
  virtual ~DiagnosticConsumer() = default;

  virtual void handleDiagnostic(const Diagnostic &Diag) = 0;
};

class DiagnosticsEngine {
public:
  DiagnosticsEngine(DiagnosticConsumer *Client) : Client(Client) {}

  unsigned getNumErrors() const { return NumErrors; }

  InFlightDiagnostic emitError(SMLoc Loc, unsigned DiagId);
  InFlightDiagnostic emitWarning(SMLoc Loc, unsigned DiagId);
  void report(SourceMgr::DiagKind Kind, SMLoc Loc, StringRef Msg);

private:
  DiagnosticConsumer *Client;
  unsigned NumWarnings = 0;
  unsigned NumErrors = 0;
};

class InFlightDiagnostic {
public:
  InFlightDiagnostic(DiagnosticsEngine *DiagEngine, SourceMgr::DiagKind Kind,
                     SMLoc Loc, unsigned DiagId)
      : DiagEngine(DiagEngine), Kind(Kind), Location(Loc), DiagId(DiagId) {}

  ~InFlightDiagnostic() { emit(); }

  InFlightDiagnostic &&operator<<(StringRef Arg) {
    NumArgs++;
    Args.emplace_back(Arg);
    return std::move(*this);
  }

private:
  void emit();

private:
  static constexpr const int MaxNumArgs = 10;

private:
  template <std::size_t... Idx>
  std::string formatMessage(std::index_sequence<Idx...>, StringRef Format) {
    return std::string(llvm::formatv(Format.data(), Args[Idx]...));
  }

private:
  DiagnosticsEngine *DiagEngine;
  SmallVector<std::string, MaxNumArgs> Args;
  SourceMgr::DiagKind Kind;
  SMLoc Location;
  unsigned DiagId;
  unsigned NumArgs = 0;
};

class TextDiagnosticPrinter final : public DiagnosticConsumer {
public:
  TextDiagnosticPrinter(SourceMgr &SrcMgr)
      : DiagnosticConsumer(), SrcMgr(SrcMgr) {}

  void handleDiagnostic(const Diagnostic &Diag);

private:
  SourceMgr &SrcMgr;
};

} // namespace chocopy
