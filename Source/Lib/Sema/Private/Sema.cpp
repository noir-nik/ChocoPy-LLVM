module Sema;
import AST;
// import Analysis;
import Basic;
import std;
import :Scope;

#define _LOG_FATAL(X) llvm::report_fatal_error(X);
#define M_FWD(...) _LOG_FATAL(__VA_ARGS__);
#define LOG_FATAL(X) llvm::report_fatal_error(X);

namespace chocopy {
static raw_ostream &operator<<(raw_ostream &Stream, const Type &T) {
  llvm::TypeSwitch<const Type *>(&T)
      .Case([&Stream](const FuncType *FT) {
        Stream << "(";
        for (ValueType *T : FT->getParametersTypes()) {
          Stream << T;
          if (T != FT->getParametersTypes().back())
            Stream << ", ";
        }
        Stream << ") -> " << FT->getReturnType();
      })
      .Case([&Stream](const ClassValueType *CVT) {
        Stream << CVT->getClassName();
      })
      .Case([&Stream](const ListValueType *LVT) {
        std::string Str;
        llvm::raw_string_ostream(Str) << *LVT->getElementType();
        Stream << llvm::formatv("[{}]", Str);
      });
  return Stream;
}

static InFlightDiagnostic &&operator<<(InFlightDiagnostic &&D, const Type &T) {
  std::string Err;
  llvm::raw_string_ostream(Err) << T;
  D << Err;
  return std::move(D);
}

class Sema::Analysis : public RecursiveASTVisitor<Analysis> {
  using Base = RecursiveASTVisitor<Analysis>;

  class SemaScope {
  public:
    SemaScope() = delete;
    SemaScope(const SemaScope &) = delete;
    SemaScope(SemaScope &&) = delete;
    SemaScope &operator=(const SemaScope &) = delete;
    SemaScope &operator=(SemaScope &&) = delete;

    SemaScope(Analysis *Self, Scope::ScopeKind Kind = Scope::ScopeKind::Global)
        : Self(Self) {
      std::shared_ptr<Scope> NewScope =
          std::make_shared<Scope>(Self->Actions.getCurScope(), Kind);
      Self->Actions.setCurScope(NewScope);
    }

    ~SemaScope() {
      std::shared_ptr<Scope> S = Self->Actions.getCurScope();
      Self->Actions.actOnPopScope(S.get());
      Self->Actions.setCurScope(S->getParent());
    }

  private:
    Analysis *Self;
  };

public:
  Analysis(Sema &Actions)
      : Actions(Actions), Diags(Actions.getDiagnosticEngine()) {}

  bool traverseProgram(Program *P) {
    SemaScope ScopeGuard(this);
    Actions.setGlobalScope(Actions.getCurScope());
    Actions.initializeGlobalScope();
    for (Declaration *D : P->getDeclarations())
      handleDeclaration(D);
    return Base::traverseProgram(P);
  }

  bool traverseClassDef(ClassDef *C) {
    SemaScope ClassScope(this, Scope::ScopeKind::Class);
    Actions.IdResolver.addDecl(C);
    if (!Actions.checkSuperClass(C)) {
      return Base::traverseClassDef(C);
    }
    ClassDef *SC = Actions.getSuperClass(C);
    for (Declaration *D : C->getDeclarations()) {
      handleDeclaration(D);
      Actions.checkClassDeclaration(SC, D);
      if (FuncDef::classof(D)) {
        Actions.checkInitDeclaration(C, dyn_cast<FuncDef>(D));
        Actions.checkFirstMethodParam(C, dyn_cast<FuncDef>(D));
      }
    }
    return Base::traverseClassDef(C);
  }

  bool traverseFuncDef(FuncDef *F) {
    Actions.handleFuncDef(F);
    bool RTC = visitClassType(dyn_cast<ClassType>(F->getReturnType()));
    std::shared_ptr<Scope> CS = Actions.GlobalScope;
    if (Actions.getCurScope() != Actions.GlobalScope)
      CS = Actions.getCurScope();
    SemaScope FunctionScope(this, Scope::ScopeKind::Func);
    Actions.CurScope->setParent(CS);
    for (ParamDecl *P : F->getParams()) {
      handleDeclaration(P);
      Actions.checkTypeAnnotation(dyn_cast<ClassType>(P->getType()));
    }
    for (Declaration *D : F->getDeclarations()) {
      handleDeclaration(D);
      if (FuncDef::classof(D))
        traverseFuncDef(dyn_cast<FuncDef>(D));
      if (VarDef::classof(D))
        Actions.checkTypeAnnotation(
            dyn_cast<ClassType>(dyn_cast<VarDef>(D)->getType()));
    }
    ValueType *ERTy = Actions.Ctx.convertAnnotationToVType(F->getReturnType());
    for (Stmt *S : F->getStatements()) {
      auto tr = Base::traverseStmt(S);
      if (ReturnStmt::classof(S) && RTC) {
        ReturnStmt *RS = dyn_cast<ReturnStmt>(S);
        Actions.checkReturnStmt(RS, ERTy);
      }
    }
    if ((ERTy->isInt() || ERTy->isBool() || ERTy->isStr()) &&
        !Actions.checkReturnMissing(F->getStatements()))
      Diags.emitError(F->getLocation().Start, diag::err_maybe_falloff_nonvoid)
          << F->getName();
    return true;
  }

  bool traverseVarDef(VarDef *V) {
    Base::traverseLiteral(V->getValue());
    Actions.actOnVarDef(V);
    return true;
  }

  bool traverseAssignStmt(AssignStmt *A) {
    Base::traverseExpr(A->getValue());
    if (ListExpr::classof(A->getValue())) {
      Actions.checkExprList(dyn_cast<ListExpr>(A->getValue()));
    }
    for (Expr *T : A->getTargets()) {
      if (T->getKind() != Expr::Kind::MemberExpr)
        Base::traverseExpr(T);
      if (Actions.checkAssignTarget(T)) {
        Actions.checkAssignment(T, A->getValue());
      }
    }
    return true;
  }

  bool traverseReturnStmt(ReturnStmt *R) {
    // Check nullptr
    if (R->getValue())
      Base::traverseExpr(R->getValue());
    if (Actions.getCurScope() == Actions.GlobalScope) {
      Diags.emitError(R->getLocation().Start, diag::err_bad_return_top);
      return false;
    }
    return true;
  }

  /// Expressions:
  bool traverseBinaryExpr(BinaryExpr *B) {
    Base::traverseExpr(B->getLeft());
    Base::traverseExpr(B->getRight());
    Actions.actOnBinaryExpr(B);
    return true;
  }

  bool traverseCallExpr(CallExpr *C) {
    for (Expr *E : C->getArgs())
      Base::traverseExpr(E);
    Actions.checkCallExpr(C);
    return true;
  }

  bool traverseListExpr(ListExpr *LE) {
    Base::traverseListExpr(LE);
    Actions.checkExprList(LE);
    return true;
  }

  bool traverseExprStmt(ExprStmt *ES) {
    if (Expr *E = ES->getExpr()) {
      if (IndexExpr *IE = dyn_cast<IndexExpr>(E)) {
        Base::traverseExpr(E);
        Actions.checkIndexExpr(IE);
      }
      if (UnaryExpr *UE = dyn_cast<UnaryExpr>(E)) {
        Base::traverseExpr(E);
        Actions.checkUnaryExpr(UE);
      }
      if (MemberExpr *ME = dyn_cast<MemberExpr>(E)) {
        Actions.checkMemberExpr(ME, false);
        return true;
      }
      if (MethodCallExpr *MC = dyn_cast<MethodCallExpr>(E)) {
        if (Actions.checkMemberExpr(MC->getMethod(), true)) {
          for (Expr *E : MC->getArgs())
            Base::traverseExpr(E);
          Actions.checkMethodCallExpr(MC);
        }
        return true;
      }
      Base::traverseExpr(E);
    }
    return true;
  }

  bool traverseDeclRef(DeclRef *DR) {
    Actions.actOnDeclRef(DR);
    return true;
  }

  bool traverseBooleanLiteral(BooleanLiteral *B) {
    B->setInferredType(Actions.Ctx.getBoolTy());
    return true;
  }

  bool traverseIntegerLiteral(IntegerLiteral *I) {
    I->setInferredType(Actions.Ctx.getIntTy());
    return true;
  }

  bool traverseNoneLiteral(NoneLiteral *N) {
    N->setInferredType(Actions.Ctx.getNoneTy());
    return true;
  }

  bool traverseStringLiteral(StringLiteral *S) {
    S->setInferredType(Actions.Ctx.getStrTy());
    return true;
  }

  bool traverseMemberExpr(MemberExpr *M) {
    Base::traverseMemberExpr(M);
    Actions.checkMemberExpr(M, false);
    return true;
  }
  /// Types
  bool visitClassType(ClassType *C) {
    if (!Actions.checkTypeAnnotation(C))
      return false;
    return true;
  }

private:
  void handleDeclaration(Declaration *D) {
    StringRef Name = D->getName();
    Scope *S = Actions.getCurScope().get();
    if (Actions.lookupName(S, D->getSymbolInfo())) {
      Diags.emitError(D->getNameId()->getLocation().Start, diag::err_dup_decl)
          << Name;
      return;
    }
    if (isa<VarDef>(D)) {
      Actions.handleDeclaration(D);
      Actions.checkClassShadow(D);
    } else if (isa<ParamDecl>(D)) {
      Actions.handleDeclaration(D);
      Actions.checkClassShadow(D);
    } else if (isa<ClassDef>(D)) {
      Actions.handleClassDef(dyn_cast<ClassDef>(D));
    } else if (isa<FuncDef>(D)) {
      Actions.handleFuncDef(dyn_cast<FuncDef>(D));
      Actions.checkClassShadow(D);
    } else if (isa<GlobalDecl>(D)) {
      Actions.checkGlobalDecl(dyn_cast<GlobalDecl>(D));
    } else if (isa<NonLocalDecl>(D)) {
      Actions.checkNonlocalDecl(dyn_cast<NonLocalDecl>(D));
    } else {
      llvm::report_fatal_error(
          "Unsupported this kind of declaration! Add support...");
    }
  }

private:
  Sema &Actions;
  DiagnosticsEngine &Diags;
};

Sema::Sema(DiagnosticsEngine &Diags, ASTContext &C) : Diags(Diags), Ctx(C) {}

void Sema::initialize() {
  ClassDef *ObjCD = Ctx.getObjectClass();
  ClassDef *IntCD = Ctx.getIntClass();
  ClassDef *StrCD = Ctx.getStrClass();
  ClassDef *BoolCD = Ctx.getBoolClass();
  ClassDef *NoneCD = Ctx.getNoneClass();

  FuncDef *PrintFD = Ctx.getPrintFunc();
  FuncDef *InputFD = Ctx.getInputFunc();
  FuncDef *LenFD = Ctx.getLenFunc();

  IdResolver.addDecl(ObjCD);
  IdResolver.addDecl(IntCD);
  IdResolver.addDecl(StrCD);
  IdResolver.addDecl(BoolCD);
  IdResolver.addDecl(NoneCD);

  IdResolver.addDecl(PrintFD);
  IdResolver.addDecl(InputFD);
  IdResolver.addDecl(LenFD);
}

void Sema::initializeGlobalScope() {
  ClassDef *ObjCD = Ctx.getObjectClass();
  ClassDef *IntCD = Ctx.getIntClass();
  ClassDef *StrCD = Ctx.getStrClass();
  ClassDef *BoolCD = Ctx.getBoolClass();
  ClassDef *NoneCD = Ctx.getNoneClass();

  FuncDef *PrintFD = Ctx.getPrintFunc();
  FuncDef *InputFD = Ctx.getInputFunc();
  FuncDef *LenFD = Ctx.getLenFunc();

  GlobalScope->addDecl(ObjCD);
  GlobalScope->addDecl(IntCD);
  GlobalScope->addDecl(StrCD);
  GlobalScope->addDecl(BoolCD);
  GlobalScope->addDecl(NoneCD);

  GlobalScope->addDecl(PrintFD);
  GlobalScope->addDecl(InputFD);
  GlobalScope->addDecl(LenFD);
}

void Sema::handleDeclaration(Declaration *D) {
  if (CurScope->isDeclInScope(D))
    return;
  CurScope->addDecl(D);
  IdResolver.addDecl(D);
}

void Sema::handleClassDef(ClassDef *C) {
  if (CurScope->isDeclInScope(C))
    return;
  CurScope->addDecl(C);
  IdResolver.addDecl(C);
}

void Sema::handleFuncDef(FuncDef *F) {
  if (CurScope->isDeclInScope(F))
    return;
  CurScope->addDecl(F);
  IdResolver.addDecl(F);
}

void Sema::run() {
  Analysis V(*this);
  V.traverseAST(Ctx);
}

void Sema::actOnPopScope(Scope *S) {
  auto Decls = S->getDecls();
  for (Declaration *D : Decls)
    IdResolver.removeDecl(D);
}

bool Sema::checkNonlocalDecl(NonLocalDecl *NLD) {
  if (CurScope->getParent().get() == GlobalScope.get()) {
    Diags.emitError(NLD->getLocation().Start, diag::err_not_nonlocal)
        << NLD->getName();
    return false;
  }
  Declaration *D =
      lookupName(CurScope->getParent().get(), NLD->getSymbolInfo());
  if (D != nullptr) {
    if (VarDef::classof(D)) {
      handleDeclaration(D);
      return true;
    } else {
      Diags.emitError(NLD->getLocation().Start, diag::err_not_nonlocal)
          << NLD->getName();
      return false;
    }
  } else {
    Diags.emitError(NLD->getLocation().Start, diag::err_not_nonlocal)
        << NLD->getName();
    return false;
  }
}

bool Sema::checkGlobalDecl(GlobalDecl *GD) {
  Declaration *D = lookupName(GlobalScope.get(), GD->getSymbolInfo());
  if (D != nullptr) {
    if (VarDef::classof(D)) {
      VarDef *V = cast<VarDef>(D);
      actOnVarDef(V);
      handleDeclaration(D);
      return true;
    } else {
      Diags.emitError(GD->getLocation().Start, diag::err_not_global)
          << D->getName();
    }
  } else {
    Diags.emitError(GD->getLocation().Start, diag::err_not_global)
        << GD->getName();
  }
  return false;
}

bool Sema::checkSuperClass(ClassDef *D) {
  SymbolInfo *CS = D->getSuperClass()->getSymbolInfo();
  auto It = IdResolver.begin(CS);
  auto Dt = IdResolver.begin(D->getSymbolInfo());
  auto Et = IdResolver.end();
  if (It == IdResolver.end()) {
    Diags.emitError(D->getSuperClass()->getLocation().Start,
                    diag::err_supclass_not_def)
        << D->getSuperClass()->getName();
    return false;
  }
  if (ClassDef *SC = dyn_cast<ClassDef>(*It)) {
    if (SC == Ctx.getIntClass() || SC == Ctx.getBoolClass() ||
        SC == Ctx.getNoneClass() || SC == Ctx.getStrClass()) {
      Diags.emitError(D->getSuperClass()->getLocation().Start,
                      diag::err_supclass_is_special_class)
          << SC->getName();
      return false;
    } else if (std::distance(It, Et) < (std::distance(Dt, Et)) &&
               SC != Ctx.getObjectClass()) {
      Diags.emitError(D->getSuperClass()->getLocation().Start,
                      diag::err_supclass_not_def)
          << D->getSuperClass()->getName();
      return false;
    }
    return true;
  } else {
    Diags.emitError(D->getSuperClass()->getLocation().Start,
                    diag::err_supclass_isnot_class)
        << dyn_cast<Declaration>(*It)->getName();
    return false;
  }
}

bool Sema::checkMethodOverride(FuncDef *Ovr, FuncDef *M) {
  ArrayRef<ParamDecl *> OP = Ovr->getParams();
  ArrayRef<ParamDecl *> P = M->getParams();
  if (OP.size() != 0 && OP[0]->getName() == "self")
    OP = OP.slice(1);
  if (P.size() != 0 && P[0]->getName() == "self")
    P = P.slice(1);
  if (OP.size() != P.size()) {
    Diags.emitError(M->getLocation().Start, diag::err_method_override)
        << Ovr->getName();
    return false;
  }
  for (auto i = 0; i < OP.size(); ++i) {
    if (Ctx.convertAnnotationToVType(OP[i]->getType()) !=
        Ctx.convertAnnotationToVType(P[i]->getType())) {
      Diags.emitError(M->getLocation().Start, diag::err_method_override)
          << Ovr->getName();
      return false;
    }
  }
  if (Ctx.convertAnnotationToVType(Ovr->getReturnType()) !=
      Ctx.convertAnnotationToVType(M->getReturnType())) {
    Diags.emitError(M->getLocation().Start, diag::err_method_override)
        << Ovr->getName();
    return false;
  }
  return true;
}

bool Sema::checkFirstMethodParam(ClassDef *ClsDef, FuncDef *FuncDef) {
  ArrayRef<ParamDecl *> P = FuncDef->getParams();
  if (!P.empty()) {
    ParamDecl *S = P[0];
    if (S->getName() != "self" || !ClassType::classof(S->getType()) ||
        dyn_cast<ClassType>(S->getType())->getClassName() !=
            ClsDef->getName()) {
      Diags.emitError(FuncDef->getNameId()->getLocation().Start,
                      diag::err_first_method_param)
          << FuncDef->getName();
      return false;
    }
  } else {
    Diags.emitError(FuncDef->getNameId()->getLocation().Start,
                    diag::err_first_method_param)
        << FuncDef->getName();
    return false;
  }
  return true;
}

bool Sema::checkParams(SMRange point, ArrayRef<ParamDecl *> Indecs,
                       ArrayRef<Expr *> Args) {
  ArrayRef<ParamDecl *> Decls;
  int addon = 0;
  if (Indecs.size() != 0 && Indecs[0]->getName() == "self") {
    Decls = Indecs.slice(1);
    addon = 1;
  } else {
    Decls = Indecs;
  }
  if (Decls.size() != Args.size()) {
    Diags.emitError(point.Start, diag::err_num_arguments)
        << std::to_string(Decls.size()) << std::to_string(Args.size());
    return false;
  }
  for (auto i = 0; i < Args.size(); ++i) {
    if (DeclRef *R = dyn_cast<DeclRef>(Args[i]))
      actOnDeclRef(R);
    ValueType *ATy = dyn_cast<ValueType>(Args[i]->getInferredType());
    ValueType *DTy = Ctx.convertAnnotationToVType(Decls[i]->getType());
    if (ATy != DTy) {
      return false;
    }
  }
  return true;
}

bool Sema::checkCallExpr(CallExpr *C) {
  Declaration *FD = nullptr;
  if (DeclRef::classof(C->getFunction())) {
    FD = lookupDecl(dyn_cast<DeclRef>(C->getFunction()));
    if (FD == nullptr) {
      Diags.emitError(C->getLocation().Start, diag::err_call)
          << dyn_cast<DeclRef>(C->getFunction())->getName();
      return false;
    }
  }
  if (FuncDef *F = dyn_cast<FuncDef>(FD)) {
    ArrayRef<ParamDecl *> decLn = F->getParams();
    ArrayRef<Expr *> argLn = C->getArgs();
    checkParams(C->getLocation(), decLn, argLn);
    C->setInferredType(Ctx.convertAnnotationToVType(F->getReturnType()));
  } else if (ClassDef *CD = dyn_cast<ClassDef>(FD)) {
    C->setInferredType(Ctx.getClassVType(CD->getName()));
  }
  return true;
}

bool Sema::checkMethodCallExpr(MethodCallExpr *Method) {
  IdentifierResolver::iterator It = IdResolver.begin(
      dyn_cast<DeclRef>(Method->getMethod()->getObject())->getSymbolInfo());
  if (VarDef *V = dyn_cast<VarDef>(*It)) {
    if (Declaration *D =
            lookupClass(GlobalScope.get(), dyn_cast<ClassType>(V->getType()))) {
      if (ClassDef *Cls = dyn_cast<ClassDef>(D)) {
        if (Declaration *D =
                findDeclaration(Cls, Method->getMethod()->getMember())) {
          if (FuncDef *Func = dyn_cast<FuncDef>(D)) {
            return checkParams(Method->getLocation(), Func->getParams(),
                               Method->getArgs());
          }
        }
      }
    }
  }
  return true;
}

bool Sema::checkAssignment(Expr *Lhs, Expr *Rhs) {
  ValueType *LTy = nullptr;
  ValueType *RTy = nullptr;
  if (Lhs->getInferredType() == Ctx.getObjectTy()) {
    return false;
  }
  if (IfExpr *IE = dyn_cast<IfExpr>(Rhs)) {
    Type *Cond = dyn_cast<BinaryExpr>(IE->getCondExpr())->getInferredType();
    Type *Then = IE->getThenExpr()->getInferredType();
    Type *Else = IE->getElseExpr()->getInferredType();
    if (Cond == dyn_cast<Type>(Ctx.getBoolTy())) {
      if (dyn_cast<ValueType>(Then) != LTy) {
        RTy = dyn_cast<ValueType>(Then);
      } else {
        RTy = dyn_cast<ValueType>(Else);
      }
    } else {
      // Diags.emitError(IE->getCondExpr()->getLocation().Start,
      //                 diag::err_cond_expr)
      // << *Cond;
      return false;
    }
  } else if (CallExpr *CE = dyn_cast<CallExpr>(Rhs)) {
    checkCallExpr(CE);
    RTy = dyn_cast<ValueType>(CE->getInferredType());
  } else if (IndexExpr *IU = dyn_cast<IndexExpr>(Rhs)) {
    checkIndexExpr(IU);
    RTy = dyn_cast<ValueType>(IU->getList()->getInferredType());
  } else {
    RTy = dyn_cast<ValueType>(Rhs->getInferredType());
  }
  if (MemberExpr *ME = dyn_cast<MemberExpr>(Lhs)) {
    LTy = dyn_cast<ValueType>(ME->getMember()->getInferredType());
  } else if (IndexExpr *IE = dyn_cast<IndexExpr>(Lhs)) {
    auto inferred = dyn_cast<ListValueType>(IE->getList()->getInferredType());
    if (!inferred) {
      Diags.emitError(IE->getLocation().Start, diag::err_cannot_index)
          << *dyn_cast<Type>(IE->getList()->getInferredType());
      return false;
    }
    LTy = inferred->getElementType();
  } else {
    LTy = dyn_cast<ValueType>(Lhs->getInferredType());
  }
  if (LTy == Ctx.getObjectTy() ||
      (ListValueType::classof(RTy) &&
       dyn_cast<ListValueType>(RTy)->getElementType() == Ctx.getNoneTy())) {
    return true;
  }
  if ((RTy != LTy) &&
      !(ListValueType::classof(LTy) && RTy == Ctx.getEmptyTy())) {
    Diags.emitError(Lhs->getLocation().Start, diag::err_tc_assign)
        << *dyn_cast<Type>(LTy) << *dyn_cast<Type>(RTy);
    return false;
  }
  return true;
}

bool Sema::checkAssignTarget(Expr *E) {
  if (E->getInferredType() == Ctx.getObjectTy()) {
    return false;
  }
  if (DeclRef *DR = dyn_cast<DeclRef>(E)) {
    auto It = IdResolver.begin(DR->getSymbolInfo());
    if (It == IdResolver.end() || !CurScope->isDeclInScope(*It)) {
      Diags.emitError(DR->getLocation().Start, diag::err_bad_local_assign)
          << DR->getName();
      return false;
    }
  } else if (IndexExpr *IE = dyn_cast<IndexExpr>(E)) {
    checkIndexExpr(IE);
    Type *LTy = IE->getList()->getInferredType();
    if (LTy == Ctx.getStrTy()) {
      Diags.emitError(IE->getLocation().Start, diag::err_tc_assign) << *LTy;
      return false;
    }
  } else if (MemberExpr *ME = dyn_cast<MemberExpr>(E)) {
    return checkMemberExpr(ME, false);
  } else {
    llvm::report_fatal_error("Unsupported assignement target! Add support...");
    return false;
  }
  return true;
}

bool Sema::checkIndexExpr(IndexExpr *E) {
  if (DeclRef::classof(E->getList())) {
    if (!IntegerLiteral::classof(E->getIndex())) {
      // Diags.emitError(I->getIndex()->getLocation().Start,
      Diags.emitError(E->getLocation().Start, diag::err_index_not_int)
          << *dyn_cast<Type>(E->getIndex()->getInferredType());
      return false;
    }
    return true;
  } else {
    if (ListExpr::classof(E->getList())) {
      checkExprList(dyn_cast<ListExpr>(E->getList()));
    }
    Diags.emitError(E->getLocation().Start, diag::err_cannot_index)
        << *dyn_cast<Type>(E->getList()->getInferredType());
    return false;
  }
}

bool Sema::checkClassDeclaration(ClassDef *C, Declaration *D) {
  for (Declaration *SD : C->getDeclarations()) {
    if (SD->getName() == D->getName() && D->getName() != "__init__") {
      if (FuncDef::classof(SD) && FuncDef::classof(D)) {
        FuncDef *OF = dyn_cast<FuncDef>(SD);
        FuncDef *NF = dyn_cast<FuncDef>(D);
        return checkMethodOverride(OF, NF);
      } else {
        Diags.emitError(D->getLocation().Start, diag::err_redefine_attr)
            << D->getName();
        return false;
      }
    }
  }
  return true;
}

bool Sema::checkMemberExpr(MemberExpr *ME, bool IsMethod) {
  Expr *O = ME->getObject();
  DeclRef *M = ME->getMember();
  if (DeclRef *D = dyn_cast<DeclRef>(O)) {
    IdentifierResolver::iterator It = IdResolver.begin(D->getSymbolInfo());
    if (ParamDecl *P = dyn_cast<ParamDecl>(*It)) {
      Declaration *D =
          lookupClass(GlobalScope.get(), dyn_cast<ClassType>(P->getType()));
      if (ClassDef *Cls = dyn_cast<ClassDef>(D)) {
        if (Declaration *ClsDecl = findDeclaration(Cls, M)) {
          if (VarDef *VD = dyn_cast<VarDef>(ClsDecl))
            M->setInferredType(
                dyn_cast<Type>(Ctx.convertAnnotationToVType(VD->getType())));
        } else {
          if (IsMethod)
            Diags.emitError(O->getLocation().Start, diag::err_method_not_exist)
                << M->getName() << Cls->getName();
          else
            Diags.emitError(O->getLocation().Start, diag::err_attr_not_exist)
                << M->getName() << Cls->getName();
          return false;
        }
      }
    }
    if (VarDef *VDef = dyn_cast<VarDef>(*It)) {
      Declaration *D =
          lookupClass(GlobalScope.get(), dyn_cast<ClassType>(VDef->getType()));
      if (ClassDef *C = dyn_cast<ClassDef>(D)) {
        if (Declaration *SD = findDeclaration(C, M)) {
          if (VarDef *VD = dyn_cast<VarDef>(SD))
            M->setInferredType(
                dyn_cast<Type>(Ctx.convertAnnotationToVType(VD->getType())));
        } else {
          if (IsMethod)
            Diags.emitError(O->getLocation().Start, diag::err_method_not_exist)
                << M->getName() << C->getName();
          else
            Diags.emitError(O->getLocation().Start, diag::err_attr_not_exist)
                << M->getName() << C->getName();
          return false;
        }
      }
    }
  }
  return true;
}

bool Sema::checkInitDeclaration(ClassDef *C, FuncDef *FD) {
  if (FD->getName() != "__init__") {
    return true;
  }
  std::vector<VarDef *> definedVars;
  for (Declaration *D : C->getDeclarations()) {
    if (VarDef::classof(D) && dyn_cast<VarDef>(D)->getValue() != nullptr) {
      definedVars.push_back(dyn_cast<VarDef>(D));
    }
  }
  ArrayRef<VarDef *> defVars(definedVars);
  for (ParamDecl *PD : FD->getParams()) {
    StringRef name = PD->getName();
    ValueType *type =
        cast<ValueType>(Ctx.convertAnnotationToVType(PD->getType()));
    for (VarDef *D : defVars) {
      ValueType *deftype =
          cast<ValueType>(Ctx.convertAnnotationToVType(D->getType()));
      if (D->getName() == name && deftype == type) {
        Diags.emitError(FD->getLocation().Start, diag::err_method_override)
            << FD->getName();
        return false;
      }
    }
  }
  return true;
}

Declaration *Sema::findDeclaration(ClassDef *C, DeclRef *M) {
  for (Declaration *D : C->getDeclarations()) {
    if (D->getName() == M->getName())
      return D;
  }
  if (ClassDef *SC = getSuperClass(C))
    return findDeclaration(SC, M);
  else
    return nullptr;
}

FuncDef *Sema::getInitFunc(ClassDef *C) {
  for (Declaration *D : C->getDeclarations()) {
    if (D->getName() == "__init__" && FuncDef::classof(D))
      return dyn_cast<FuncDef>(D);
  }
  if (ClassDef *SC = getSuperClass(C))
    return getInitFunc(SC);
  else
    return nullptr;
}

bool Sema::checkUnaryExpr(UnaryExpr *UE) {
  Type *OTy = dyn_cast<Type>(UE->getOperand()->getInferredType());
  if (UE->getOpKind() == UnaryExpr::OpKind::Not && OTy != Ctx.getBoolTy()) {
    Diags.emitError(UE->getLocation().Start, diag::err_tc_unary)
        << "not" << *OTy;
    return false;
  }
  if (UE->getOpKind() == UnaryExpr::OpKind::Minus && OTy != Ctx.getIntTy()) {
    Diags.emitError(UE->getLocation().Start, diag::err_tc_unary) << "-" << *OTy;
    return false;
  }
  return true;
}

bool Sema::checkExprList(ListExpr *LE) {
  if (LE->getElements().empty()) {
    LE->setInferredType(dyn_cast<Type>(Ctx.getEmptyTy()));
    return true;
  } else {
    ArrayRef<Expr *> vec = LE->getElements();
    Type *supp_type = vec[0]->getInferredType();
    for (const auto &elem : vec) {
      if (elem->getInferredType() != supp_type) {
        Diags.emitError(elem->getLocation().Start, diag::err_cannot_index)
            << *supp_type;
        return false;
      }
    }
    LE->setInferredType(
        dyn_cast<Type>(Ctx.getListVType(dyn_cast<ValueType>(supp_type))));
    return true;
  }
}

bool Sema::checkReturnStmt(ReturnStmt *S, ValueType *ERTy) {
  Expr *RE = dyn_cast<ReturnStmt>(S)->getValue();
  Expr *RetExpr = dyn_cast<ReturnStmt>(S)->getValue();
  ValueType *RTy = nullptr;
  if (!RetExpr || RetExpr->getInferredType() == nullptr) {
    RTy = Ctx.getNoneTy();
  } else if (MemberExpr *ME = dyn_cast<MemberExpr>(RetExpr)) {
    RTy = dyn_cast<ValueType>(ME->getMember()->getInferredType());
  } else {
    RTy = dyn_cast<ValueType>(RetExpr->getInferredType());
  }
  if (((ERTy->isInt() || ERTy->isBool() || ERTy->isStr()) && ERTy != RTy) ||
      (ERTy != RTy && !RTy->isNone())) {
    Diags.emitError(S->getLocation().Start, diag::err_tc_assign)
        << *dyn_cast<Type>(ERTy) << *dyn_cast<Type>(RTy);
    return false;
  }
  return true;
}

bool Sema::checkReturnMissing(ArrayRef<Stmt *> SL) {
  for (Stmt *S : SL) {
    if (IfStmt::classof(S)) {
      IfStmt *IS = dyn_cast<IfStmt>(S);
      return checkReturnMissing(IS->getThenBody()) &&
             checkReturnMissing(IS->getElseBody());
    }
    if (ReturnStmt::classof(S))
      return true;
  }
  return false;
}

bool Sema::checkTypeAnnotation(ClassType *C) {
  Scope *S = getCurScope().get();
  Declaration *D =
      lookupClass(S, C) ? lookupClass(S, C) : lookupClass(GlobalScope.get(), C);
  if (!D || !ClassDef::classof(D)) {
    if (D && (D->getName() == "bool" || D->getName() == "int" ||
              D->getName() == "str"))
      return true;
    Diags.emitError(C->getLocation().Start, diag::err_invalid_type_annotation)
        << C->getClassName();
    return false;
  }
  return true;
}

void Sema::actOnVarDef(VarDef *V) {
  auto &RTy = *cast<ValueType>(V->getValue()->getInferredType());
  auto &LTy = *cast<ValueType>(Ctx.convertAnnotationToVType(V->getType()));
  if (ClassType::classof(V->getType())) {
    ClassType *CT = cast<ClassType>(V->getType());
    checkTypeAnnotation(CT);
  }
  if (!(RTy <= LTy))
    Diags.emitError(V->getLocation().Start, diag::err_tc_assign) << LTy << RTy;
}

void Sema::actOnBinaryExpr(BinaryExpr *B) {
  Type *LopTy = B->getLeft()->getInferredType();
  Type *RopTy = B->getRight()->getInferredType();
  ValueType &LTy = LopTy ? *cast<ValueType>(LopTy) : *Ctx.getObjectTy();
  ValueType &RTy = RopTy ? *cast<ValueType>(RopTy) : *Ctx.getObjectTy();

  bool Err = false;

  switch (B->getOpKind()) {
  case BinaryExpr::OpKind::Add:
    if (LTy.isInt() || RTy.isInt()) {
      Err = &LTy != &RTy;
      B->setInferredType(Ctx.getIntTy());
    } else if (LTy.isStr() || RTy.isStr()) {
      Err = &LTy != &RTy;
      B->setInferredType(Ctx.getStrTy());
    } else {
      auto *LListTy = dyn_cast<ListValueType>(&LTy);
      auto *RListTy = dyn_cast<ListValueType>(&RTy);
      ValueType *Result = nullptr;
      if (LListTy && RListTy) {
        ValueType *Ty = RListTy->getElementType();
        Result = Ctx.getListVType(Ty);
      } else {
        Err = true;
        Result = Ctx.getObjectTy();
      }
      B->setInferredType(Result);
    }
    break;
  case BinaryExpr::OpKind::Sub:
  case BinaryExpr::OpKind::Mul:
    if (LTy.isInt() || RTy.isInt()) {
      Err = &LTy != &RTy;
      B->setInferredType(Ctx.getIntTy());
    } else {
      auto *LListTy = dyn_cast<ListValueType>(&LTy);
      auto *RListTy = dyn_cast<ListValueType>(&RTy);
      if (LListTy && RListTy)
        Err = LListTy->getElementType() != RListTy->getElementType();
      else
        Err = true;
      B->setInferredType(Err ? static_cast<ValueType *>(Ctx.getObjectTy())
                             : static_cast<ValueType *>(LListTy));
    }
    break;
  case BinaryExpr::OpKind::Mod:
  case BinaryExpr::OpKind::FloorDiv:
    Err = !LTy.isInt() || !RTy.isInt();
    B->setInferredType(Ctx.getIntTy());
    break;

  case BinaryExpr::OpKind::And:
  case BinaryExpr::OpKind::Or:
    Err = !LTy.isBool() || !RTy.isBool();
    B->setInferredType(Ctx.getBoolTy());
    break;

  case BinaryExpr::OpKind::EqCmp:
  case BinaryExpr::OpKind::NEqCmp:
    Err = (!LTy.isStr() && !LTy.isInt() && !LTy.isBool()) || (&LTy != &RTy);
    B->setInferredType(Ctx.getBoolTy());
    break;

  case BinaryExpr::OpKind::LEqCmp:
  case BinaryExpr::OpKind::GEqCmp:
  case BinaryExpr::OpKind::LCmp:
  case BinaryExpr::OpKind::GCmp:
    Err = !LTy.isInt() || !RTy.isInt();
    B->setInferredType(Ctx.getBoolTy());
    break;

  case BinaryExpr::OpKind::Is:
    Err = LTy.isStr() || LTy.isInt() || LTy.isBool() || RTy.isStr() ||
          RTy.isInt() || RTy.isBool();
    B->setInferredType(Ctx.getBoolTy());
    break;
  }

  if (Err) {
    Diags.emitError(B->getLocation().Start, diag::err_tc_binary)
        << B->getOpKindStr() << LTy << RTy;
  }
}

void Sema::checkClassShadow(Declaration *ID) {
  SymbolInfo *SI = ID->getSymbolInfo();
  IdentifierResolver::iterator I = IdResolver.begin(SI);
  IdentifierResolver::iterator E = IdResolver.end();
  Declaration *D = nullptr;
  for (; !D && I != E; ++I) {
    if (ClassDef *C = dyn_cast<ClassDef>(*I)) {
      if (C->getName() == ID->getName()) {
        Diags.emitError(ID->getLocation().Start, diag::err_bad_shadow)
            << C->getName();
        return;
      }
    }
  }
}

void Sema::actOnDeclRef(DeclRef *DR) {
  Declaration *D = lookupDecl(DR);

  if (!D || isa<FuncDef>(D)) {
    SMLoc Loc = DR->getLocation().Start;
    Diags.emitError(Loc, diag::err_not_variable) << DR->getName();
    DR->setInferredType(Ctx.getObjectTy());
    return;
  }

  auto convertToValueTy = [DR, this](auto *D) {
    ValueType *VT = Ctx.convertAnnotationToVType(D->getType());
    DR->setDeclInfo(D);
    DR->setInferredType(VT);
  };

  llvm::TypeSwitch<Declaration *>(D)
      .Case<ParamDecl>(convertToValueTy)
      .Case<VarDef>(convertToValueTy)
      .Default([](auto) {
        llvm::report_fatal_error("Unsupported declaration! Add support...");
      });
}

Scope *Sema::getScopeForDecl(Scope *S, Declaration *D) {
  do {
    if (S->isDeclInScope(D))
      return S;
  } while ((S = S->getParent().get()));
  return nullptr;
}

ClassDef *Sema::getSuperClass(ClassDef *CD) {
  if (CD == Ctx.getObjectClass())
    return nullptr;
  SymbolInfo *CS = CD->getSuperClass()->getSymbolInfo();
  IdentifierResolver::iterator It = IdResolver.begin(CS);
  if (It != IdResolver.end()) {
    return dyn_cast<ClassDef>(*It);
  }
  return nullptr;
}

Declaration *Sema::lookupClass(Scope *S, ClassType *CT) {
  auto Decls = S->getDecls();
  auto It = llvm::find_if(Decls, [CT](Declaration *D) {
    return D->getName() == CT->getClassName();
  });
  if (It != Decls.end())
    return *It;
  return nullptr;
}

Declaration *Sema::lookupName(Scope *S, SymbolInfo *SI) {
  auto Decls = S->getDecls();
  auto It = llvm::find_if(
      Decls, [SI](Declaration *D) { return D->getSymbolInfo() == SI; });
  if (It != Decls.end())
    return *It;
  return nullptr;
}

Declaration *Sema::lookupDecl(DeclRef *DR) {
  SymbolInfo *SI = DR->getSymbolInfo();
  IdentifierResolver::iterator I = IdResolver.begin(SI);
  IdentifierResolver::iterator E = IdResolver.end();

  Declaration *D = nullptr;

  for (; !D && I != E; ++I) {
    if (GlobalDecl *GD = dyn_cast<GlobalDecl>(*I)) {
      D = lookupName(GlobalScope.get(), GD->getSymbolInfo());
      return cast<VarDef>(D);
    }

    if (isa<NonLocalDecl>(*I))
      continue;
    D = *I;
  }

  return D;
}
} // namespace chocopy
