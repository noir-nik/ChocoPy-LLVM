export module ASCIICharInfo;
import std;

export namespace chocopy {
namespace ascii {
extern const std::uint16_t gInfoTable[256];

enum {
	CHAR_HORZ_WS = 0x0001, // '\t', '\f', '\v'.  Note, no '\0'
	CHAR_VERT_WS = 0x0002, // '\r', '\n'
	CHAR_SPACE   = 0x0004, // ' '
	CHAR_DIGIT   = 0x0008, // 0-9
	CHAR_XLETTER = 0x0010, // a-f,A-F
	CHAR_UPPER   = 0x0020, // A-Z
	CHAR_LOWER   = 0x0040, // a-z
	CHAR_UNDER   = 0x0080, // _
	CHAR_PERIOD  = 0x0100, // .
	CHAR_PUNCT   = 0x0200, // {}[]#<>%:;?*+-/^&|~!=,"'`$@()
};

enum {
	CHAR_XUPPER = CHAR_XLETTER | CHAR_UPPER,
	CHAR_XLOWER = CHAR_XLETTER | CHAR_LOWER
};
} // namespace ascii

inline bool IsASCII(unsigned char c) { return c <= 127; }

inline bool IsASCII(char c) { return c <= 127; }

/// Returns true if this character is horizontal ASCII whitespace:
/// ' ', '\\t', '\\f', '\\v'.
///
/// Note that this returns false for '\\0'.
inline bool IsHorizontalWhitespace(unsigned char c) {
	using namespace ascii;
	return (gInfoTable[c] & (CHAR_HORZ_WS | CHAR_SPACE)) != 0;
}

/// Returns true if this character is vertical ASCII whitespace: '\\n', '\\r'.
///
/// Note that this returns false for '\\0'.
inline bool IsVerticalWhitespace(unsigned char c) {
	using namespace ascii;
	return (gInfoTable[c] & CHAR_VERT_WS) != 0;
}

/// Return true if this character is horizontal or vertical ASCII whitespace:
/// ' ', '\\t', '\\f', '\\v', '\\n', '\\r'.
///
/// Note that this returns false for '\\0'.
inline bool IsWhitespace(unsigned char c) {
	using namespace ascii;
	return (gInfoTable[c] & (CHAR_HORZ_WS | CHAR_VERT_WS | CHAR_SPACE)) != 0;
}

/// Return true if this character is an ASCII digit: [0-9]
inline bool IsDigit(unsigned char c) {
	using namespace ascii;
	return (gInfoTable[c] & CHAR_DIGIT) != 0;
}

/// Return true if this character is a lowercase ASCII letter: [a-z]
inline bool IsLowercase(unsigned char c) {
	using namespace ascii;
	return (gInfoTable[c] & CHAR_LOWER) != 0;
}

/// Return true if this character is an uppercase ASCII letter: [A-Z]
inline bool IsUppercase(unsigned char c) {
	using namespace ascii;
	return (gInfoTable[c] & CHAR_UPPER) != 0;
}

/// Return true if this character is an ASCII letter: [a-zA-Z]
inline bool IsLetter(unsigned char c) {
	using namespace ascii;
	return (gInfoTable[c] & (CHAR_UPPER | CHAR_LOWER)) != 0;
}

/// Return true if this character is an ASCII letter or digit: [a-zA-Z0-9]
inline bool IsAlphanumeric(unsigned char c) {
	using namespace ascii;
	return (gInfoTable[c] & (CHAR_DIGIT | CHAR_UPPER | CHAR_LOWER)) != 0;
}

/// Return true if this character is an ASCII hex digit: [0-9a-fA-F]
inline bool IsHexDigit(unsigned char c) {
	using namespace ascii;
	return (gInfoTable[c] & (CHAR_DIGIT | CHAR_XLETTER)) != 0;
}

/// Return true if this character is an ASCII punctuation character.
///
/// Note that '_' is both a punctuation character and an identifier character!
inline bool IsPunctuation(unsigned char c) {
	using namespace ascii;
	return (gInfoTable[c] & (CHAR_UNDER | CHAR_PERIOD | CHAR_PUNCT)) != 0;
}

/// Return true if this character is an ASCII printable character; that is, a
/// character that should take exactly one column to print in a fixed-width
/// terminal.
inline bool IsPrintable(unsigned char c) {
	using namespace ascii;
	return (gInfoTable[c] & (CHAR_UPPER | CHAR_LOWER | CHAR_PERIOD | CHAR_PUNCT |
							CHAR_DIGIT | CHAR_UNDER | CHAR_SPACE)) != 0;
}

/// Return true if this is the body character of a C preprocessing number,
/// which is [a-zA-Z0-9_.].
inline bool IsPreprocessingNumberBody(unsigned char c) {
	using namespace ascii;
	return (gInfoTable[c] & (CHAR_UPPER | CHAR_LOWER | CHAR_DIGIT | CHAR_UNDER |
							CHAR_PERIOD)) != 0;
}

/// Return true if this is the body character of a C++ raw string delimiter.
inline bool IsRawStringDelimBody(unsigned char c) {
	using namespace ascii;
	return (gInfoTable[c] & (CHAR_UPPER | CHAR_LOWER | CHAR_PERIOD | CHAR_DIGIT |
							CHAR_UNDER | CHAR_PUNCT)) != 0 &&
		   c != '(' && c != ')' && c != '\\';
}

/// Return true if this is the head of chocopy identifiers, which is [a-zA-Z_].
inline bool IsChocopyIdentifierHead(unsigned char c) {
	using namespace ascii;
	return (gInfoTable[c] & (CHAR_UPPER | CHAR_LOWER | CHAR_UNDER)) != 0;
}

/// Return true if this is the chocopy identifiers, which is [a-zA-Z0-9_].
inline bool IsChocopyIdentifierBody(unsigned char c) {
	using namespace ascii;
	return (gInfoTable[c] & (CHAR_UPPER | CHAR_LOWER | CHAR_DIGIT | CHAR_UNDER)) != 0;
}
} // namespace chocopy
