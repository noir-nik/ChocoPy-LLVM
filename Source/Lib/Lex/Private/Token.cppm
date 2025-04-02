export module Token;
import std;

enum class TokenKind : unsigned short {
#define TOK(X) X,
#include "TokenKinds.def"
	MaxEnum
};

export struct Token {
	TokenKind        kind;
	std::string_view text;
};
