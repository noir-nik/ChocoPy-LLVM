module;
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/GraphTraits.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/Hashing.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/PointerIntPair.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/ADT/Twine.h>
#include <llvm/ADT/TypeSwitch.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/SMLoc.h>
#include <llvm/Support/SaveAndRestore.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/VersionTuple.h>


export module LLVM;

// NOLINTBEGIN(misc-unused-using-decls)
export namespace llvm {
using llvm::BumpPtrAllocator;
using llvm::DenseMap;
using llvm::DenseMapInfo;
using llvm::DenseSet;
using llvm::errs;
using llvm::find_if;
using llvm::format;
using llvm::formatv;
using llvm::GraphTraits;
using llvm::hash_combine;
using llvm::hash_combine_range;
using llvm::Inverse;
using llvm::iterator_range;
using llvm::make_const_ptr;
using llvm::make_range;
using llvm::nulls;
using llvm::outs;
using llvm::PointerIntPair;
using llvm::raw_string_ostream;
using llvm::report_fatal_error;
using llvm::reverse;
using llvm::SmallPtrSet;
using llvm::SmallString;
using llvm::SmallVector;
using llvm::SMLoc;
using llvm::SMRange;
using llvm::SourceMgr;
using llvm::StringMap;
using llvm::StringMapEntry;
using llvm::StringRef;
using llvm::StringSwitch;
using llvm::Twine;
using llvm::TypeSwitch;

using llvm::APInt;
using llvm::isa;
using llvm::TypeSwitch;

using llvm::operator==;
using llvm::operator!=;
using llvm::operator<=;
using llvm::operator>=;
using llvm::operator<;
using llvm::operator>;
using llvm::operator+;
// using llvm::operator-;
// using llvm::operator*;
// using llvm::operator/;
// using llvm::operator%;
// using llvm::operator|;
// using llvm::operator^;
// using llvm::operator&;
using llvm::operator<<;
// using llvm::operator>>;

} // namespace llvm

export using ::operator new;
export using ::operator delete;

export namespace llvm::json {
using llvm::json::OStream;
};

export namespace chocopy {
// Casting operators.
using llvm::cast;
using llvm::cast_if_present;
using llvm::cast_or_null;
using llvm::dyn_cast;
using llvm::dyn_cast_if_present;
using llvm::dyn_cast_or_null;
using llvm::isa;
using llvm::isa_and_nonnull;
using llvm::isa_and_present;

// ADT's.
using llvm::ArrayRef;
using llvm::MutableArrayRef;
using llvm::OwningArrayRef;
using llvm::SaveAndRestore;
using llvm::SmallString;
using llvm::SmallVector;
using llvm::SmallVectorImpl;
using llvm::SMRange;
using llvm::StringRef;
using llvm::Twine;
using llvm::VersionTuple;

// Error handling.
using llvm::Expected;

// Reference counting.
using llvm::IntrusiveRefCntPtr;
using llvm::IntrusiveRefCntPtrInfo;
using llvm::RefCountedBase;

using llvm::raw_ostream;
using llvm::raw_pwrite_stream;
} // namespace chocopy
// NOLINTEND(misc-unused-using-decls)
