module;

#include <llvm/Support/ErrorHandling.h>

export module AST:StmtVisitor;
import Basic;

import :AST;

// #include <llvm/ADT/STLExtras.h>
export namespace chocopy {
namespace stmtvisitor {

template <template <typename> class Ptr, typename ImplClass,
          typename RetTy = void>
class Base {
public:
#define PTR(CLASS) typename Ptr<CLASS>::type
#define DISPATCH(NAME, CLASS)                                                  \
  return static_cast<ImplClass *>(this)->visit##NAME(static_cast<PTR(CLASS)>(S))

  // clang-format off
  RetTy visit(PTR(Stmt) S) {
    switch (S->getKind()) {
#define STMT(CLASS, KIND)                                                      \
  case Stmt::StmtKind::KIND:  DISPATCH(CLASS, CLASS);
#include "StmtNodes.def"
    }

    llvm_unreachable("Unsupported statement!");
    return RetTy();
  }
  // clang-format on

  // clang-format off
  // Default implementation
  RetTy visitAssignStmt(PTR(AssignStmt) S) {  DISPATCH(Stmt, Stmt); }
  RetTy visitExprStmt(PTR(ExprStmt) S) {      DISPATCH(Stmt, Stmt); }
  RetTy visitForStmt(PTR(ForStmt) S) {        DISPATCH(Stmt, Stmt); }
  RetTy visitIfStmt(PTR(IfStmt) S) {          DISPATCH(Stmt, Stmt); }
  RetTy visitReturnStmt(PTR(ReturnStmt) S) {  DISPATCH(Stmt, Stmt); }
  RetTy visitWhileStmt(PTR(WhileStmt) S) {    DISPATCH(Stmt, Stmt); }
  RetTy visitStmt(PTR(Stmt) ) {               return RetTy(); }
  // clang-format on

#undef PTR
#undef DISPATCH
};
} // namespace stmtvisitor

template <typename Impl, typename RetTy = void>
class StmtVisitor : public stmtvisitor::Base<std::add_pointer, Impl, RetTy> {};

template <typename Impl, typename RetTy = void>
class ConstStmtVisitor
    : public stmtvisitor::Base<llvm::make_const_ptr, Impl, RetTy> {};
} // namespace chocopy

