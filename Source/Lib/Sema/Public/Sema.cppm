export module Sema;
import AST;
import Basic;
export import :IdentifierResolver;
export import :Scope;

export namespace chocopy {

class Sema {
  class Analysis;

public:
  Sema(DiagnosticsEngine &Diags, ASTContext &C);

  void initialize();
  void initializeGlobalScope();

public:
  DiagnosticsEngine &getDiagnosticEngine() const { return Diags; }

  void run();

private:
  std::shared_ptr<Scope> getGlobalScope() const { return GlobalScope; }
  void setGlobalScope(std::shared_ptr<Scope> S) { GlobalScope = std::move(S); }

  std::shared_ptr<Scope> getCurScope() const { return CurScope; }
  void setCurScope(std::shared_ptr<Scope> S) { CurScope = std::move(S); }

  void handleDeclaration(Declaration *D);

  void actOnPopScope(Scope *S);

  bool checkNonlocalDecl(NonLocalDecl *NLD);
  bool checkGlobalDecl(GlobalDecl *GD);
  bool checkSuperClass(ClassDef *D);
  bool checkClassAttrs(ClassDef *D);
  bool checkMethodOverride(FuncDef *OM, FuncDef *M);
  bool checkClassDef(ClassDef *D);
  bool checkFirstMethodParam(ClassDef *CD, FuncDef *FD);
  bool checkAssignTarget(Expr *E);
  bool checkReturnStmt(ReturnStmt *S);
  bool checkReturnMissing(FuncDef *F);
  bool checkTypeAnnotation(ClassType *T);

  void actOnVarDef(VarDef *V);
  void actOnBinaryExpr(BinaryExpr *B);
  void actOnDeclRef(DeclRef *DR);

  Scope *getScopeForDecl(Scope *S, Declaration *D);

  ClassDef *getSuperClass(ClassDef *CD);

  bool isSameType(TypeAnnotation *TyA, TypeAnnotation *TyB);

  Declaration *lookupName(Scope *S, SymbolInfo *SI);
  Declaration *lookupDecl(DeclRef *DR);

private:
  using Nonlocals = SmallVector<NonLocalDecl *, 4>;
  using Globals = SmallVector<GlobalDecl *, 4>;

private:
  DiagnosticsEngine &Diags;
  ASTContext &Ctx;
  std::shared_ptr<Scope> GlobalScope;
  std::shared_ptr<Scope> CurScope;
  IdentifierResolver IdResolver;
};
} // namespace chocopy
