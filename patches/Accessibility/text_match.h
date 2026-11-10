// Engine-free text comparisons, for matching engine-rendered text against
// our own TLK resolution of the same strref.

#pragma once

namespace acc::text {

// Case-sensitive compare that ignores leading/trailing whitespace on both
// sides. The compare to reach for whenever engine-RENDERED text is matched
// against our own LookupTlk resolution of the same strref: the two are not
// byte-identical, because the engine pads captions when it builds the
// gui_string. KOTOR 2 renders every close button as " Close" against a
// TLK that says "Close", which is exactly how a plain strcmp let the
// close-button chain filter fail on every K2 sub-screen. (The 0x11 glyph
// marker that K2 puts ahead of that pad space is NOT whitespace and is not
// this function's job — ReadGuiString removes it at the read.)
bool EqualsTrimmed(const char* a, const char* b);

// EqualsTrimmed for lines the engine token-substitutes at render time
// (<FullName> and friends): the stored text's literal segments must appear
// left to right in the rendered line, a token matches any run of characters,
// and padding spaces at either end do not count. Without a token it is
// EqualsTrimmed.
bool MatchesWithTokens(const char* rendered, const char* stored);

}  // namespace acc::text
