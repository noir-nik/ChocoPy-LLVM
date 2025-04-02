export module TokenKinds;

export namespace chocopy {
namespace tok {
enum TokenKind : unsigned short {
#define TOK(X) X,
#include "TokenKinds.def"
	NUM_TOKENS
};

const char* getTokenName(TokenKind Kind);
const char* getPunctuatorSpelling(TokenKind Kind);

[[maybe_unused]] constexpr bool IsLiteral(TokenKind Kind) {
	return Kind == integer_literal || Kind == idstring ||
		   Kind == string_literal;
}

[[maybe_unused]] constexpr bool IsBinOp(TokenKind Kind) {
	return Kind == plus || Kind == minus || Kind == star ||
		   Kind == slashslash || Kind == percent ||
		   Kind == equalequal || Kind == exclaimequal ||
		   Kind == lessequal || Kind == greaterequal ||
		   Kind == less || Kind == greater || Kind == kw_is;
}

[[maybe_unused]] constexpr bool IsKeyword(TokenKind Kind) {
	switch (Kind) {
#define KEYWORD(ID, STR) case kw_##ID:
#include "TokenKinds.def"
		return true;
	default:
		return false;
	}
}

[[maybe_unused]] constexpr bool IsPunctuator(TokenKind Kind) {
	switch (Kind) {
#define PUNCTUATOR(ID, STR) case ID:
#include "TokenKinds.def"
		return true;
	default:
		return false;
	}
}
} // namespace tok
} // namespace chocopy
