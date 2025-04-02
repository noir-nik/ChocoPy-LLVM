export module Lexer;
import std;

import Token;

export namespace chocopy {
class Lexer {
	bool GetNextToken(Token& token);

private:
	inline bool IsEof() { return ptr == buffer.end(); }
	inline auto ReadNext() -> char const* { return ptr++; }

	void SkipWhitespaces();

	// Buffer view
	std::string_view buffer;
	// Current position in the buffer
	char* ptr = nullptr;
};

} // namespace chocopy
