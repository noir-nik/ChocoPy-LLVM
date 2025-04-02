module Lexer;
import ASCIICharInfo;

static constexpr char Space[] = " \t\v\r\n";

namespace chocopy {

// void Lexer::SkipWhitespaces() {
// 	while (IsSpace(*ptr)) {
// 		readNext();
// 	}
// }

bool Lexer::GetNextToken(Token& in_token) {
	using namespace chocopy::ascii;
	Token token;
	while (!IsEof()) {
		int indention = 0;

		while (!IsEof()) {
			if (*ptr == ' ')
				indention += 1;
			else if (*ptr == '\t')
				indention += 8 - indention % 8;
			else
				break;
			ReadNext();
		}

		if (*ptr == '#') {
			do {
				ReadNext();
			} while (!IsEof() && !IsVerticalWhitespace(*ptr));
		}

		if (IsVerticalWhitespace(*ptr)) {
			ReadNext();
			continue;
		}

		if (IsEof())
			break;
	}

	in_token = token;
	return true;
}
} // namespace chocopy
