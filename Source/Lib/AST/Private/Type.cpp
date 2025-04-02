module AST;
import :Type;
import :ASTContext;
namespace chocopy {
bool ValueType::isInt() const { return this == Ctx.getIntTy(); }

bool ValueType::isStr() const { return this == Ctx.getStrTy(); }
bool ValueType::isBool() const { return this == Ctx.getBoolTy(); }

bool ValueType::isNone() const { return this == Ctx.getNoneTy(); }

bool operator<=(const ValueType &Sub, const ValueType &Sup) {
  return Sub.Ctx.isAssignementCompatibility(&Sub, &Sup);
}
} // namespace chocopy
