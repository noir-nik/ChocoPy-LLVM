export module Parser;
import AST;
import Basic;
import Lexer;
import Sema;
import std;

export namespace chocopy {
struct TypedVar;

class Parser {
  class ParseScope;

public:
  Parser(ASTContext &C, Lexer &Lex, Sema &Acts);

  Program *parse();

private:
  bool consumeToken(tok::TokenKind ExpectedTok);
  bool consumeToken();

  bool expect(tok::TokenKind ExpectedTok);
  bool expectAndConsume(tok::TokenKind ExpectedTok);

  void skipToNextLine();

  void emitUnexpected();

  const Token &getLookAheadToken(int N);

  Program *parseProgram();
  Declaration *parseDeclaration();
  ClassDef *parseClassDef();
  bool parseClassBody(DeclList &Members);
  FuncDef *parseFuncDef();
  bool parseTypedVar(TypedVar &T);
  // Identifier *parseTypedVar();
  bool parseFuncBody(DeclList &Declarations, StmtList &Statements);
  GlobalDecl *parseGlobalDecl();
  NonLocalDecl *parseNonlocalDecl();
  Stmt *parseStmt();
  Stmt *parseSimpleStmt();
  // Provide StartLoc to skip if token in case of elif
  Stmt *parseIfStmt(SMLoc StartLoc = SMLoc());
  Stmt *parseWhileStmt();
  Stmt *parseForStmt();
  bool parseBlock(StmtList &Statements);
  Expr *parseExpr();
  Expr *parseExprPrecedence1(); // expr1 ::= expr2 [ if expr2 else expr1 ]*
  Expr *parseExprPrecedence2(); // expr2 ::= expr3 [ or expr3 ]*
  Expr *parseExprPrecedence3(); // expr3 ::= expr4 [ and expr4 ]*
  Expr *parseExprPrecedence4(); // expr4 ::= not expr4 | cexpr5
  Expr *parseCExpr();
  Expr *parseCExprPrecedence5(); // cexpr5 ::= cexpr6 [ [==, !=, <=, >=, <, >,
                                 // is ] cexpr6 ]*
  Expr *parseCExprPrecedence6(); // cexpr6 ::= cexpr7 [ [+ | -] cexpr7 ]*
  Expr *parseCExprPrecedence7(); // expr7 ::= expr8 [ [* | // | %] expr8 ]*
  Expr *parseCExprPrecedence8(); // expr8 ::= -cexpr8 | cexpr

  Expr *parseMemberExpr(Expr *Object); // member or method call
  Expr *parseIndexExpr(Expr *Left);  // index_expr ::= cexpr [ expr ]
  Expr *parseFnCall();

  bool ParseExprList(ExprList &Args, tok::TokenKind StopToken);

  Expr *parsePrimaryExpr();
  Expr *parseTarget();
  Stmt *parseAssignOrExpr();

  Expr *parseBinaryAddOrSub();
  Expr *parseBinaryMulOrDivOrMod();
  Expr *parseAtomicExpr();

  TypeAnnotation *parseType();
  VarDef *parseVarDef();
  Literal *parseLiteral();
  bool IsVarDef(Token &Tok);

  ParamDeclList parseFuncArgList();
  bool IsNotPassStmt(Stmt *S);

private:
  DiagnosticsEngine &Diags;
  ASTContext &Context;
  Lexer &TheLexer;
  Token Tok;

  // Not in AST
  struct PassStmt : public Stmt {
    auto setLocation(SMRange NewLoc) -> std::decay_t<decltype(*this)> & {
      Loc = NewLoc;
      return *this;
    }
    PassStmt() : Stmt(SMRange(), Stmt::StmtKind::ExprStmt) {}

  public:
    using Stmt::Stmt;

  } PassStmt;
};

} // namespace chocopy
