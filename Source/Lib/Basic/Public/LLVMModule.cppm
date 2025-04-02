module;
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/SMLoc.h>
#include <llvm/Support/SourceMgr.h>

export module LLVM;

// NOLINTBEGIN(misc-unused-using-decls)
export namespace llvm {
using llvm::formatv;
using llvm::SmallString;
using llvm::SmallVector;
using llvm::SMLoc;
using llvm::SMRange;
using llvm::SourceMgr;
using llvm::StringMap;
using llvm::StringRef;
using llvm::StringSwitch;
using llvm::Twine;

using llvm::BumpPtrAllocator;
using llvm::SmallVector;
using llvm::StringMapEntry;

} // namespace llvm
// NOLINTEND(misc-unused-using-decls)
