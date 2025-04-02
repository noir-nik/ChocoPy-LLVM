module AST;
import :AST;
import Basic;
namespace chocopy {
void Declaration::dump(ASTContext &C) const {
  JSONDumper Dumper(C);
  Dumper.visit(this);
}
} // namespace chocopy
