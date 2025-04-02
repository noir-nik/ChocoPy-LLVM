module;

#include <llvm/Support/ErrorHandling.h>

export module AST:TypeAnnotationVisitor;
import Basic;
import :AST;

export namespace chocopy {
namespace typeannotationvisitor {

template <template <typename> class Ptr, typename ImplClass,
          typename RetTy = void>
class Base {
public:
#define PTR(CLASS) typename Ptr<CLASS>::type
#define DISPATCH(NAME, CLASS)                                                  \
  return static_cast<ImplClass *>(this)->visit##NAME(static_cast<PTR(CLASS)>(T))

  // clang-format off
  RetTy visit(PTR(TypeAnnotation) T) {
    switch (T->getKind()) {
#define TYPE_ANNOTATION(CLASS, KIND)                                                      \
  case TypeAnnotation::Kind::KIND:  DISPATCH(CLASS, CLASS);
#include "TypeAnnotationNodes.def"
    }

    llvm_unreachable("Unsupported type!");
    return RetTy();
  }
  // clang-format on

  // clang-format off
  // Default implementation
  RetTy visitClassType(PTR(ClassType) T) {  DISPATCH(TypeAnnotation, TypeAnnotation); }
  RetTy visitListType(PTR(ListType) T) {    DISPATCH(TypeAnnotation, TypeAnnotation); }

  RetTy visitTypeAnnotation(PTR(TypeAnnotation) ) { return RetTy(); }
  // clang-format on

#undef PTR
#undef DISPATCH
};
} // namespace typeannotationvisitor

template <typename Impl>
class TypeAnnotationVisitor
    : public typeannotationvisitor::Base<std::add_pointer, Impl> {};

template <typename Impl>
class ConstTypeAnnotationVisitor
    : public typeannotationvisitor::Base<llvm::make_const_ptr, Impl> {};
} // namespace chocopy
