module Sema;
import AST;
import :Scope;

namespace chocopy {
void Scope::addDecl(Declaration *D) { Decls.insert(D); }
} // namespace chocopy