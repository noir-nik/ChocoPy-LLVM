export module Parser;
import AST;
import Basic;
import Lexer;
import Sema;

export namespace chocopy {

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

  Program *parseProgramDemo();
  Stmt *parseStmtDemo();
  Stmt *parseAssignOrExprDemo();
  Expr *parseExprDemo();
  Expr *parseBinaryAddOrSubDemo();
  Expr *parseBinaryMulOrDivOrModDemo();
  Expr *parseFnCallDemo();
  Expr *parseAtomicExprDemo();

  TypeAnnotation *parseType();
  VarDef *parseVarDef();
  Literal *parseLiteral();

private:
  DiagnosticsEngine &Diags;
  ASTContext &Context;
  Lexer &TheLexer;
  Token Tok;
};
} // namespace chocopy
