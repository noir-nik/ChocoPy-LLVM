module;

#include <cassert>

#include <llvm/Support/ErrorHandling.h>

module Parser;
import Basic;
import AST;
import std;

namespace chocopy {
auto BinOpKindFromToken(Token Tok) -> BinaryExpr::OpKind;

struct TypedVar {
  TypeAnnotation *Type;
  SymbolInfo *Name;
  SMRange Loc;
};

struct Parser::ParseScope {
  DeclList &Decls;
  StmtList &Stmts;
  ParseScope(DeclList &Decls, StmtList &Stmts) : Decls(Decls), Stmts(Stmts) {}
  ~ParseScope() {
    Decls.clear();
    Stmts.clear();
  }
};

// Stmt *PassStmt = (Stmt *)(~0ULL);

constexpr char const *NonTypeStr = "<None>";

bool Parser::isNotPassStmt(Stmt *S) { return S != &PassStmt; }

bool Parser::isVarDef(Token &Tok) {
  return Tok.isOneOf(tok::identifier, tok::idstring) &&
         TheLexer.LookAhead(0).is(tok::colon);
};

Parser::Parser(ASTContext &C, Lexer &Lex, Sema &Acts)
    : Diags(Lex.getDiagnostics()), Context(C), TheLexer(Lex) {}

Program *Parser::parse() {
  Program *P = parseProgram();
  return P;
}

bool Parser::consumeToken(tok::TokenKind ExpectedTok) {
  if (Tok.is(ExpectedTok)) {
    consumeToken();
    return true;
  }
  return false;
}

bool Parser::consumeToken() {
  auto PrintTok = [this]() {
    std::printf("Consuming ");
    Tok.print();
    std::printf("\n");
  };
  // PrintTok();
  TheLexer.lex(Tok);
  return true;
}

bool Parser::expect(tok::TokenKind ExpectedTok) {
  if (Tok.is(ExpectedTok))
    return true;
  Diags.emitError(Tok.getLocation().Start, diag::err_near_token) << Tok;
  Diags.emitError(Tok.getLocation().Start, diag::err_expected)
      << tok::getTokenName(ExpectedTok);
  return false;
}

bool Parser::expectAndConsume(tok::TokenKind ExpectedTok) {
  return expect(ExpectedTok) && consumeToken();
}

void Parser::skipToNextLine() {
  while (!Tok.is(tok::eof) && !consumeToken(tok::NEWLINE))
    consumeToken();
}

void Parser::emitUnexpected() {
  Diags.emitError(Tok.getLocation().Start, diag::err_unexpected) << Tok;
}

const Token &Parser::getLookAheadToken(int N) {
  assert(N);
  return TheLexer.LookAhead(N - 1);
}

bool Parser::isDeclaration(Token &Tok) {
  return isVarDef(Tok) || Tok.is(tok::kw_def) || Tok.is(tok::kw_class);
}

// program ::= declaration* stmt*
Program *Parser::parseProgram() {
  DeclList Declarations;
  StmtList Statements;

  consumeToken();

  // Parse declarations
  while (isDeclaration(Tok)) {
    if (Declaration *D = parseDeclaration()) {
      Declarations.push_back(D);
    } else {
      skipToNextLine();
    }
  }

  // Parse statements
  while (!Tok.is(tok::eof)) {
    if (Stmt *S = parseStmt()) {
      if (isNotPassStmt(S)) {
        Statements.push_back(S);
      }
    } else {
      skipToNextLine();
    }
  }

  return Context.createProgram(Declarations, Statements);
}

// declaration ::= [var def | func def | class def]
Declaration *Parser::parseDeclaration() {
  // Parse variable definition
  if (isVarDef(Tok)) {
    return parseVarDef();
  }

  // Parse function definition
  if (Tok.is(tok::kw_def)) {
    return parseFuncDef();
  }

  // Parse class definition
  if (Tok.is(tok::kw_class)) {
    return parseClassDef();
  }
  return nullptr;
}

// class_def ::= class ID ( ID ) : NEWLINE INDENT class_body DEDENT
ClassDef *Parser::parseClassDef() {
  SMLoc StartLoc = Tok.getLocation().Start;

  if (!consumeToken(tok::kw_class))
    return nullptr;

  if (!expect(tok::identifier))
    return nullptr;

  SymbolInfo *ClassName = Tok.getSymbolInfo();
  SMRange ClassNameLoc = Tok.getLocation();
  consumeToken();

  if (!expectAndConsume(tok::l_paren))
    return nullptr;

  if (!expect(tok::identifier))
    return nullptr;

  SymbolInfo *SuperName = Tok.getSymbolInfo();
  SMRange SuperNameLoc = Tok.getLocation();
  consumeToken();

  if (!expectAndConsume(tok::r_paren) || !expectAndConsume(tok::colon) ||
      !expectAndConsume(tok::NEWLINE) || !expectAndConsume(tok::INDENT))
    return nullptr;

  // Parse class body (may be empty)
  DeclList Members;
  if (!parseClassBody(Members))
    return nullptr;

  if (!expectAndConsume(tok::DEDENT))
    return nullptr;

  SMLoc EndLoc;
  if (Members.empty()) {
    EndLoc = PassStmt.getLocation().End;
  } else {
    EndLoc = Members.back()->getLocation().End;
  }
  // Actually correct
  SMRange Loc(StartLoc, EndLoc);
  // SMRange Loc(StartLoc, Tok.getLocation().Start);

  Identifier *ClassName_ID = Context.createIdentifier(ClassNameLoc, ClassName);
  Identifier *SuperName_ID = Context.createIdentifier(SuperNameLoc, SuperName);

  return Context.createClassDef(Loc, ClassName_ID, SuperName_ID, Members);
}

// class_body ::= pass NEWLINE | [var_def | func_def ]+
// return false on error
bool Parser::parseClassBody(DeclList &Members) {
  bool FoundMember = false;
  // Check for 'pass' case
  if (consumeToken(tok::kw_pass)) {
    expectAndConsume(tok::NEWLINE);
    FoundMember = true;
  }

  // Parse var_def and func_def
  while (!Tok.is(tok::DEDENT)) {
    if (isVarDef(Tok)) {
      if (VarDef *V = parseVarDef()) {
        Members.push_back(V);
        FoundMember = true;
      }
    } else if (Tok.is(tok::kw_def)) {
      FuncDef *F = parseFuncDef();
      if (!F)
        return false;
      Members.push_back(F);
      FoundMember = true;
    } else {
      return false;
    }
    // @todo: process pass
  }

  return FoundMember;
}

// func_def ::= 'def' ID '(' [typed_var [, typed_var ]*]? ')' ['->' type]? ':'
// NEWLINE INDENT func_body DEDENT
FuncDef *Parser::parseFuncDef() {
  SMLoc StartLoc = Tok.getLocation().Start;
  if (!consumeToken(tok::kw_def))
    return nullptr;

  if (!expect(tok::identifier))
    return nullptr;

  SymbolInfo *FuncName = Tok.getSymbolInfo();
  SMRange FuncNameLoc = Tok.getLocation();
  consumeToken();

  if (!expectAndConsume(tok::l_paren))
    return nullptr;

  // Parse parameters
  ParamDeclList Params;
  if (!Tok.is(tok::r_paren)) {
    do {
      TypedVar T;
      if (!parseTypedVar(T)) {
        return nullptr;
      }
      // @todo: Check name conflict
      Identifier *Ident = Context.createIdentifier(T.Loc, T.Name);
      // SMRange Loc(T.Loc.Start, T.Type->getLocation().End);
      SMRange Loc(T.Loc.Start, Tok.getLocation().Start);
      ParamDecl *Param = Context.createParamDecl(Loc, Ident, T.Type);
      Params.push_back(Param);
    } while (consumeToken(tok::comma));
  }

  if (!expectAndConsume(tok::r_paren))
    return nullptr;

  // Parse optional return type
  TypeAnnotation *ReturnType = nullptr;
  if (consumeToken(tok::arrow)) {
    ReturnType = parseType();
    if (!ReturnType)
      return nullptr;
  } else {
    SMRange Loc(Tok.getLocation().Start, Tok.getLocation().Start);
    ReturnType = Context.createClassType(Loc, NonTypeStr);
  }

  if (!expectAndConsume(tok::colon) || !expectAndConsume(tok::NEWLINE) ||
      !expectAndConsume(tok::INDENT))
    return nullptr;

  // Parse function body
  DeclList Declarations;
  StmtList Statements;
  if (!parseFuncBody(Declarations, Statements))
    return nullptr;
  SMLoc EndLoc;
  if (!Statements.empty()) {
    EndLoc = Statements.back()->getLocation().End;
  } else if (!Declarations.empty()) {
    EndLoc = Declarations.back()->getLocation().End;
  } else {
    // pass in function
    EndLoc = Tok.getLocation().Start;
  }

  if (!expectAndConsume(tok::DEDENT))
    return nullptr;

  SMRange Loc(StartLoc, EndLoc);

  Identifier *Name = Context.createIdentifier(FuncNameLoc, FuncName);

  return Context.createFuncDef(Loc, Name, Params, ReturnType, Declarations,
                               Statements);
}

// typed_var ::= ID ':' type
bool Parser::parseTypedVar(TypedVar &T) {
  if (!expect(tok::identifier))
    return false;

  T.Name = Tok.getSymbolInfo();
  T.Loc = Tok.getLocation();
  consumeToken();

  if (!expectAndConsume(tok::colon))
    return false;

  T.Type = parseType();
  if (!T.Type)
    return false;
  return true;
}

// func_body ::= [global_decl | nonlocal_decl | var_def | func_def]* stmt+
bool Parser::parseFuncBody(DeclList &Declarations, StmtList &Statements) {
  // Parse declarations
  while (true) {
    if (GlobalDecl *G = parseGlobalDecl()) {
      Declarations.push_back(G);
    } else if (NonLocalDecl *N = parseNonlocalDecl()) {
      Declarations.push_back(N);
    }
    if (isVarDef(Tok)) {
      if (VarDef *V = parseVarDef()) {
        Declarations.push_back(V);
      }
    } else if (FuncDef *F = parseFuncDef()) {
      Declarations.push_back(F);
    } else {
      break;
    }
  }

  // Parse statements (at least one)
  if (Stmt *S = parseStmt()) {
    if (isNotPassStmt(S)) {
      Statements.push_back(S);
    }
  } else {
    return false;
  }

  // Parse remaining statements
  while (!Tok.is(tok::DEDENT)) {
    if (Stmt *S = parseStmt()) {
      if (isNotPassStmt(S)) {
        Statements.push_back(S);
      }
    } else {
      skipToNextLine();
    }
  }

  return true;
}

// global_decl ::= 'global' ID NEWLINE
GlobalDecl *Parser::parseGlobalDecl() {
  SMLoc StartLoc = Tok.getLocation().Start;

  if (!consumeToken(tok::kw_global))
    return nullptr;

  if (!expect(tok::identifier))
    return nullptr;

  SymbolInfo *Name = Tok.getSymbolInfo();
  SMRange NameLoc = Tok.getLocation();
  consumeToken();

  if (!expectAndConsume(tok::NEWLINE))
    return nullptr;

  // SMLoc EndLoc = Tok.getLocation().End;
  SMRange Loc(StartLoc, NameLoc.End);

  Identifier *ID = Context.createIdentifier(NameLoc, Name);

  return Context.createGlobalDecl(Loc, ID);
}

// nonlocal_decl ::= 'nonlocal' ID NEWLINE
NonLocalDecl *Parser::parseNonlocalDecl() {
  SMLoc StartLoc = Tok.getLocation().Start;

  if (!consumeToken(tok::kw_nonlocal))
    return nullptr;

  if (!expect(tok::identifier))
    return nullptr;

  SymbolInfo *Name = Tok.getSymbolInfo();
  SMRange NameLoc = Tok.getLocation();
  consumeToken();

  if (!expectAndConsume(tok::NEWLINE))
    return nullptr;

  Identifier *ID = Context.createIdentifier(NameLoc, Name);

  SMRange Loc(StartLoc, NameLoc.End);
  return Context.createNonLocalDecl(Loc, ID);
}

// stmt ::= simple_stmt NEWLINE | if_stmt | while_stmt | for_stmt
Stmt *Parser::parseStmt() {
  switch (Tok.getKind()) {
  case tok::kw_if:
    return parseIfStmt();
  case tok::kw_while:
    return parseWhileStmt();
  case tok::kw_for:
    return parseForStmt();
  default:
    // Simple statement
    Stmt *S = parseSimpleStmt();
    if (S && expectAndConsume(tok::NEWLINE))
      return S;
    return nullptr;
  }
}

// simple_stmt ::= 'pass' | expr | 'return' [expr]? | [target '=']+ expr
Stmt *Parser::parseSimpleStmt() {
  SMRange StartLoc = Tok.getLocation();

  // Handle 'pass'
  if (consumeToken(tok::kw_pass)) {
    // SMLoc EndLoc = Tok.getLocation().End;
    // SMRange Loc(StartLoc, EndLoc);
    // return Context.createPassStmt(Loc);
    this->PassStmt.setLocation(StartLoc);
    return &this->PassStmt;
    // PassStmt.IsSet = true;
  }

  // Handle 'return' [expr]?
  if (consumeToken(tok::kw_return)) {
    Expr *RetVal = nullptr;
    if (!Tok.is(tok::NEWLINE)) {
      RetVal = parseExpr();
      if (!RetVal)
        return nullptr;
    }

    SMLoc EndLoc = Tok.getLocation().End;
    // SMLoc EndLoc = Tok.getLocation().Start;
    SMRange Loc(StartLoc.Start, EndLoc);
    return Context.createReturnStmt(Loc, RetVal);
  }

  // Parse assignment or expression statement
  return parseAssignOrExpr();
}

// if_stmt ::= if expr : block [elif expr : block]* [else : block]?
Stmt *Parser::parseIfStmt(bool IsElif) {
  SMLoc StartLoc = Tok.getLocation().Start;
  if (IsElif) {
    if (!expectAndConsume(tok::kw_elif))
      return nullptr;
  } else {
    if (!expectAndConsume(tok::kw_if))
      return nullptr;
  }

  // Parse condition
  Expr *Condition = parseExpr();
  if (!Condition)
    return nullptr;

  if (!expectAndConsume(tok::colon))
    return nullptr;

  // Parse if block
  StmtList ThenBlock;
  if (!parseBlock(ThenBlock))
    return nullptr;

  StmtList ElseBlock;

  // Parse optional elif/else clauses
  if (Tok.is(tok::kw_elif)) {
    // Treat elif as a nested if statement in the else branch
    Stmt *ElifStmt = parseIfStmt(true);
    if (!ElifStmt)
      return nullptr;

    // Add the elif statement to the else body list
    ElseBlock.push_back(ElifStmt);
  } else if (consumeToken(tok::kw_else)) {
    // Parse else block
    if (!expectAndConsume(tok::colon))
      return nullptr;

    // Parse the else block directly into ElseBlock
    if (!parseBlock(ElseBlock))
      return nullptr;
  }

  // Create the if statement with optional else branch
  SMLoc EndLoc;
  // actually correct
  // SMLoc EndLoc = !ElseBlock.empty() ? ElseBlock.back()->getLocation().End
  //                                   : Tok.getLocation().End;
  EndLoc = Tok.getLocation().Start;
  SMRange Loc(StartLoc, EndLoc);

  return Context.createIfStmt(Loc, Condition, ThenBlock, ElseBlock);
}

// while_stmt ::= 'while' expr ':' block
Stmt *Parser::parseWhileStmt() {
  SMLoc StartLoc = Tok.getLocation().Start;

  if (!expectAndConsume(tok::kw_while))
    return nullptr;

  // Parse condition
  Expr *Condition = parseExpr();
  if (!Condition)
    return nullptr;

  if (!expectAndConsume(tok::colon))
    return nullptr;

  // Parse block
  StmtList Body;
  if (!parseBlock(Body))
    return nullptr;

  SMLoc EndLoc = Tok.getLocation().Start;
  SMRange Loc(StartLoc, EndLoc);

  return Context.createWhileStmt(Loc, Condition, Body);
}

// for_stmt ::= 'for' ID 'in' expr ':' block
Stmt *Parser::parseForStmt() {
  SMLoc StartLoc = Tok.getLocation().Start;

  if (!expectAndConsume(tok::kw_for))
    return nullptr;

  // Parse iterator variable
  if (!expect(tok::identifier))
    return nullptr;

  SymbolInfo *Name = Tok.getSymbolInfo();
  SMRange NameLoc = Tok.getLocation();
  consumeToken();

  if (!expectAndConsume(tok::kw_in))
    return nullptr;

  // Parse iterable expression
  Expr *Iterable = parseExpr();
  if (!Iterable)
    return nullptr;
  // @todo(sema) check that Iterable is in fact iterable

  if (!expectAndConsume(tok::colon))
    return nullptr;

  // Parse block
  StmtList Body;
  if (!parseBlock(Body))
    return nullptr;

  // SMLoc EndLoc = Body.back()->getLocation().End; // Actualy correct
  SMLoc EndLoc = Tok.getLocation().Start;
  SMRange Loc(StartLoc, EndLoc);

  DeclRef *ID = Context.createDeclRef(NameLoc, Name);

  return Context.createForStmt(Loc, ID, Iterable, Body);
}

// block ::= NEWLINE INDENT stmt+ DEDENT
bool Parser::parseBlock(StmtList &Statements) {
  if (!expectAndConsume(tok::NEWLINE) || !expectAndConsume(tok::INDENT))
    return false;

  // Parse at least one statement
  if (Stmt *S = parseStmt()) {
    if (isNotPassStmt(S)) {
      Statements.push_back(S);
    }
  } else {
    return false;
  }

  // Parse remaining statements
  while (!Tok.is(tok::DEDENT)) {
    if (Stmt *S = parseStmt()) {
      if (isNotPassStmt(S)) {
        Statements.push_back(S);
      }
    } else {
      skipToNextLine();
    }
  }

  return expectAndConsume(tok::DEDENT);
}

// expr ::= cexpr
//        | not expr
//        | expr [and | or] expr
//        | expr if expr else expr
Expr *Parser::parseExpr() {
  // Parse the first operand
  return parseExprPrecedence1();
}

// expr1 ::= expr2 [ if expr2 else expr1 ]*
Expr *Parser::parseExprPrecedence1() {
  SMLoc StartLoc = Tok.getLocation().Start;
  Expr *Left = parseExprPrecedence2(); // if
  if (!Left)
    return nullptr;
  while (Tok.is(tok::kw_if)) {
    consumeToken();
    Expr *Condition = parseExprPrecedence2();
    if (!Condition)
      return nullptr;
    if (!expectAndConsume(tok::kw_else))
      return nullptr;
    Expr *Else = parseExprPrecedence1();
    if (!Else)
      return nullptr;
    SMRange Loc(StartLoc, Tok.getLocation().End);
    Left = Context.createIfExpr(Loc, Condition, Left, Else);
  }
  return Left;
}

// expr2 ::= expr3 [ or expr3 ]*
Expr *Parser::parseExprPrecedence2() {
  SMLoc StartLoc = Tok.getLocation().Start;
  Expr *Left = parseExprPrecedence3(); // and
  if (!Left)
    return nullptr;
  while (Tok.is(tok::kw_or)) {
    consumeToken();
    Expr *Right = parseExprPrecedence3();
    if (!Right)
      return nullptr;
    SMRange Loc(StartLoc, Right->getLocation().End);
    Left = Context.createBinaryExpr(Loc, Left, BinaryExpr::OpKind::Or, Right);
  }
  return Left;
}

// expr3 ::= expr4 [ and expr4 ]*
Expr *Parser::parseExprPrecedence3() {
  SMLoc StartLoc = Tok.getLocation().Start;
  Expr *Left = parseExprPrecedence4(); // or
  if (!Left)
    return nullptr;
  while (Tok.is(tok::kw_and)) {
    consumeToken();
    Expr *Right = parseExprPrecedence4();
    if (!Right)
      return nullptr;
    SMRange Loc(StartLoc, Tok.getLocation().End);
    Left = Context.createBinaryExpr(Loc, Left, BinaryExpr::OpKind::And, Right);
  }
  return Left;
}

// expr4 ::= not expr4 | cexpr5
Expr *Parser::parseExprPrecedence4() {
  if (Tok.is(tok::kw_not)) {
    SMLoc StartLoc = Tok.getLocation().Start;
    consumeToken();
    Expr *Right = parseExprPrecedence4(); // expr4
    if (!Right)
      return nullptr;
    SMRange Loc(StartLoc, Tok.getLocation().End);
    return Context.createUnaryExpr(Loc, UnaryExpr::OpKind::Not, Right);
  }
  return parseCExprPrecedence5(); // cexpr5
}

// cexpr5 ::= cexpr6 [ [==, !=, <=, >=, <, >, is ] cexpr6 ]*
Expr *Parser::parseCExprPrecedence5() {
  SMLoc StartLoc = Tok.getLocation().Start;
  Expr *Left = parseCExprPrecedence6();
  if (!Left)
    return nullptr;
  while (Tok.isOneOf(tok::equalequal, tok::exclaimequal, tok::lessequal,
                     tok::greaterequal, tok::less, tok::greater, tok::kw_is)) {
    BinaryExpr::OpKind Op = BinOpKindFromToken(Tok);
    consumeToken();
    Expr *Right = parseCExprPrecedence6();
    if (!Right)
      return nullptr;
    SMRange Loc(StartLoc, Right->getLocation().End);
    Left = Context.createBinaryExpr(Loc, Left, Op, Right);
  }
  return Left;
}

// cexpr6 ::= cexpr7 [ [+ | -] cexpr7 ]*
Expr *Parser::parseCExprPrecedence6() {
  SMLoc StartLoc = Tok.getLocation().Start;
  Expr *L = parseCExprPrecedence7();
  if (!L)
    return nullptr;

  while (Tok.isOneOf(tok::plus, tok::minus)) {
    BinaryExpr::OpKind K = BinOpKindFromToken(Tok);
    consumeToken();

    Expr *Right = parseCExprPrecedence7();
    if (!Right)
      return nullptr;

    SMRange Loc(StartLoc, Right->getLocation().End);
    L = Context.createBinaryExpr(Loc, L, K, Right);
  }
  return L;
}

// expr7 ::= expr8 [ [* | // | %] expr8 ]*
Expr *Parser::parseCExprPrecedence7() {
  SMLoc StartLoc = Tok.getLocation().Start;
  Expr *L = parseCExprPrecedence8();
  if (!L)
    return nullptr;

  while (Tok.isOneOf(tok::star, tok::slashslash, tok::percent)) {
    BinaryExpr::OpKind K = BinOpKindFromToken(Tok);
    consumeToken();

    Expr *Right = parseCExprPrecedence8();
    if (!Right)
      return nullptr;
    SMRange Loc(StartLoc, Right->getLocation().End);
    L = Context.createBinaryExpr(Loc, L, K, Right);
  }
  return L;
}

// expr8 ::= -cexpr8 | cexpr
Expr *Parser::parseCExprPrecedence8() {
  if (Tok.is(tok::minus)) {
    SMLoc StartLoc = Tok.getLocation().Start;
    // SMRange Loc = Tok.getLocation();
    consumeToken();
    Expr *Right = parseCExprPrecedence8(); // cexpr8
    if (!Right)
      return nullptr;

    SMRange Loc(StartLoc, Tok.getLocation().End);
    return Context.createUnaryExpr(Loc, UnaryExpr::OpKind::Minus, Right);
  }
  return parseCExpr();
}

// cexpr ::= ID
//         | literal                            // Primary
//         | [ [expr [, expr ]*]? ]             // Primary
//         | ( expr )                           // Primary
//      -> | member_expr                        // cexpr . ID
//      -> | member_expr ( [expr [, expr ]*]? ) // member_call
//      -> | index_expr                         // cexpr [ expr ]
//      -> | ID ( [expr [, expr ]*]? )          // function_call
//         | cexpr bin_op cexpr                 // parseCExprPrecedence6
//         | - cexpr                            // parseCExprPrecedence8
Expr *Parser::parseCExpr() {
  SMLoc StartLoc = Tok.getLocation().Start;

  // literal
  // [ [expr [, expr ]*]? ]
  // ( expr )
  Expr *Left = parsePrimaryExpr();
  if (!Left)
    return nullptr;

  // Parse member access, indexing and function calls

  //    Tok
  //     ∨
  // Left.ID().ID(expr, expr)
  // Left[expr][expr]
  // Left(expr, expr)(expr, expr)
  while (true) {
    // Member access
    if (Tok.is(tok::period)) {
      consumeToken();

      Left = parseMemberExpr(Left);
      if (!Left)
        return nullptr;

      // Indexing
    } else if (Tok.is(tok::l_square)) {
      Left = parseIndexExpr(Left);
      if (!Left)
        return nullptr;

      // Function call
    } else if (Tok.is(tok::l_paren)) {
      consumeToken();
      ExprList Args;
      if (!parseExprList(Args, tok::r_paren)) {
        return nullptr;
      }

      SMRange Loc(StartLoc, Tok.getLocation().Start);
      Left = Context.createCallExpr(Loc, Left, Args);
    } else {
      break;
    }
  }
  return Left;
}

// expr_list ::= [ expr [, expr ]* ]* 'StopToken'
bool Parser::parseExprList(ExprList &Args, tok::TokenKind StopToken) {
  if (Tok.isNot(StopToken)) {
    do {
      // @todo(sema): type check
      Expr *Elem = parseExpr();
      if (!Elem)
        return false;
      Args.push_back(Elem);
    } while (consumeToken(tok::comma));
  }
  if (!expectAndConsume(StopToken))
    return false;
  return true;
};

Expr *Parser::parseMemberExpr(Expr *Object) {
  if (!expect(tok::identifier))
    return nullptr;

  SymbolInfo *Member = Tok.getSymbolInfo();
  SMRange MemberLoc = Tok.getLocation();
  consumeToken();

  DeclRef *MemberID = Context.createDeclRef(MemberLoc, Member);
  SMRange Loc(Object->getLocation().Start, MemberLoc.End);

  MemberExpr *M = Context.createMemberExpr(Loc, Object, MemberID);

  // Check for method call
  if (Tok.is(tok::l_paren)) {
    consumeToken();
    ExprList Args;
    if (!parseExprList(Args, tok::r_paren)) {
      return nullptr;
    }

    SMRange CallLoc(Object->getLocation().Start, Tok.getLocation().Start);
    return Context.createMethodCallExpr(CallLoc, M, Args);
  }
  return M;
}

Expr *Parser::parseIndexExpr(Expr *Left) {
  if (!expectAndConsume(tok::l_square))
    return nullptr;
  Expr *Index = parseExpr();
  if (!Index)
    return nullptr;

  SMLoc RSquareEnd = Tok.getLocation().End;
  if (!expectAndConsume(tok::r_square))
    return nullptr;

  // SMRange Loc(Left->getLocation().Start, Tok.getLocation().Start);
  SMRange Loc(Left->getLocation().Start, RSquareEnd);
  return Context.createIndexExpr(Loc, Left, Index);
}

// primary_expr := ID | literal | list | [ expr ]
Expr *Parser::parsePrimaryExpr() {
  SMLoc StartLoc = Tok.getLocation().Start;

  switch (Tok.getKind()) {
  // ID
  case tok::identifier: {
    SymbolInfo *Name = Tok.getSymbolInfo();
    SMRange NameLoc = Tok.getLocation();
    consumeToken();
    Expr *ID = Context.createDeclRef(NameLoc, Name);
    return ID;
  }

  // Literal
  case tok::kw_None:
  case tok::kw_True:
  case tok::kw_False:
  case tok::integer_literal:
  case tok::idstring:
  case tok::string_literal:
    return parseLiteral();

  // [ expr, ... ]
  case tok::l_square: {
    consumeToken();
    ExprList Elements;
    if (!parseExprList(Elements, tok::r_square)) {
      return nullptr;
    };

    // SMRange Loc(StartLoc, Tok.getLocation().End);
    SMRange Loc(StartLoc, Tok.getLocation().Start);
    return Context.createListExpr(Loc, Elements);
  }

  // ( expr )
  case tok::l_paren: {
    consumeToken();

    Expr *E = parseExpr();
    if (!E)
      return nullptr;

    if (!expectAndConsume(tok::r_paren))
      return nullptr;

    return E;
  }

  default:
    emitUnexpected();
    return nullptr;
  }
}

// target ::= ID | member_expr | index_expr
Stmt *Parser::parseAssignOrExpr() {
  // @todo: add other targets
  auto IsTarget = [](const Expr *E) {
    return llvm::isa<DeclRef, MemberExpr, IndexExpr>(E);
  };

  SMLoc SLoc = Tok.getLocation().Start;
  ExprList Targets;
  Expr *E = nullptr;
  do {
    if (E)
      Targets.push_back(E);

    E = parseExpr();
    if (!E)
      return nullptr;
  } while (IsTarget(E) && consumeToken(tok::equal));

  if (!expect(tok::NEWLINE))
    return nullptr;

  SMLoc ELoc = Tok.getLocation().Start;
  SMRange Loc(SLoc, ELoc);
  if (!Targets.empty())
    return Context.createAssignStmt(Loc, Targets, E);

  return Context.createExprStmt(Loc, E);
}

/// type = ID | IDSTRING | '[' type ']'
TypeAnnotation *Parser::parseType() {
  SMRange Loc = Tok.getLocation();
  switch (Tok.getKind()) {
  case tok::identifier: {
    StringRef Name = Tok.getSymbolInfo()->getName();
    consumeToken();
    return Context.createClassType(Loc, Name);
  }
  case tok::idstring: {
    StringRef Name = Tok.getLiteralData();
    consumeToken();
    return Context.createClassType(Loc, Name);
  }
  case tok::l_square: {
    consumeToken();
    if (TypeAnnotation *T = parseType()) {
      if (expectAndConsume(tok::r_square)) {
        Loc = SMRange(Loc.Start, Tok.getLocation().End);
        return Context.createListType(Loc, T);
      }
    }
    return nullptr;
  }
  default:
    return nullptr;
  }
}

/// var_def = typed_var '=' literal NEWLINE
VarDef *Parser::parseVarDef() {
  SMLoc NameLoc = Tok.getLocation().Start;
  TypedVar T;

  if (!parseTypedVar(T))
    return nullptr;

  if (!expectAndConsume(tok::equal))
    return nullptr;

  if (Literal *L = parseLiteral()) {
    if (expectAndConsume(tok::NEWLINE)) {
      Identifier *V = Context.createIdentifier(T.Loc, T.Name);
      SMRange Loc(NameLoc, L->getLocation().End);
      return Context.createVarDef(Loc, V, T.Type, L);
    }
  }

  return nullptr;
}

Literal *Parser::parseLiteral() {
  SMRange Loc = Tok.getLocation();

  if (consumeToken(tok::kw_None)) {
    return Context.createNoneLiteral(Loc);
  } else if (consumeToken(tok::kw_True)) {
    return Context.createBooleanLiteral(Loc, true);
  } else if (consumeToken(tok::kw_False)) {
    return Context.createBooleanLiteral(Loc, false);
  } else if (Tok.is(tok::integer_literal)) {
    llvm::APInt Value(32, Tok.getLiteralData(), 10);
    consumeToken();
    return Context.createIntegerLiteral(Loc, Value.getSExtValue());
  } else if (Tok.isOneOf(tok::idstring, tok::string_literal)) {
    StringRef Str = Tok.getLiteralData();
    consumeToken();
    // Loc.End = SMLoc::getFromPointer(Loc.End.getPointer() - 1);
    return Context.createStringLiteral(Loc, Str);
  }

  Diags.emitError(Tok.getLocation().Start, diag::err_near_token) << Tok;
  return nullptr;
}

auto BinOpKindFromToken(Token Tok) -> BinaryExpr::OpKind {
  switch (Tok.getKind()) {
  case tok::plus:
    return BinaryExpr::OpKind::Add;
    break;
  case tok::minus:
    return BinaryExpr::OpKind::Sub;
    break;
  case tok::star:
    return BinaryExpr::OpKind::Mul;
    break;
  case tok::slashslash:
    return BinaryExpr::OpKind::FloorDiv;
    break;
  case tok::percent:
    return BinaryExpr::OpKind::Mod;
    break;
  case tok::equalequal:
    return BinaryExpr::OpKind::EqCmp;
    break;
  case tok::exclaimequal:
    return BinaryExpr::OpKind::NEqCmp;
    break;
  case tok::lessequal:
    return BinaryExpr::OpKind::LEqCmp;
    break;
  case tok::greaterequal:
    return BinaryExpr::OpKind::GEqCmp;
    break;
  case tok::less:
    return BinaryExpr::OpKind::LCmp;
    break;
  case tok::greater:
    return BinaryExpr::OpKind::GCmp;
    break;
  case tok::kw_is:
    return BinaryExpr::OpKind::Is;
    break;
  default:
    llvm_unreachable("Unexpected token for binary operator");
  }
}

} // namespace chocopy
