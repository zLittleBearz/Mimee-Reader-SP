#pragma once

#include <cstdint>
#include <string>
#define REPLACEMENT_GLYPH 0xFFFD

uint32_t utf8NextCodepoint(const unsigned char** string);
// Append one Unicode codepoint using UTF-8 encoding.
void utf8AppendCodepoint(uint32_t cp, std::string& out);
// Remove the last UTF-8 codepoint from a std::string and return the new size.
size_t utf8RemoveLastChar(std::string& str);
// Truncate string by removing N UTF-8 codepoints from the end.
void utf8TruncateChars(std::string& str, size_t numChars);

// Canonical composition (NFC) for the Latin / Vietnamese range: precomposes a
// base letter followed by combining diacritical mark(s) into a single codepoint.
// Needed because the device fonts have no combining-mark positioning, so text
// stored in NFD (e.g. some EPUB chapter titles) otherwise renders broken.
std::string utf8ComposeNfc(const std::string& in);

// Truncate a raw char buffer to the last complete UTF-8 codepoint boundary.
// Returns the new length (<= len). If the buffer ends mid-sequence, the
// incomplete trailing bytes are excluded.
int utf8SafeTruncateBuffer(const char* buf, int len);

// Returns true for CJK characters that allow line breaks on either side without hyphenation.
// Covers CJK Unified Ideographs, Hiragana, Katakana, Hangul Syllables, CJK punctuation,
// and fullwidth forms — the ranges where word boundaries are implicit per character.
inline bool utf8IsCjkBreakable(const uint32_t cp) {
  return (cp >= 0x3000 && cp <= 0x303F)        // CJK Symbols and Punctuation
         || (cp >= 0x3040 && cp <= 0x309F)     // Hiragana
         || (cp >= 0x30A0 && cp <= 0x30FF)     // Katakana
         || (cp >= 0x3400 && cp <= 0x4DBF)     // CJK Extension A
         || (cp >= 0x4E00 && cp <= 0x9FFF)     // CJK Unified Ideographs
         || (cp >= 0xAC00 && cp <= 0xD7AF)     // Hangul Syllables
         || (cp >= 0xF900 && cp <= 0xFAFF)     // CJK Compatibility Ideographs
         || (cp >= 0xFE30 && cp <= 0xFE4F)     // CJK Compatibility Forms
         || (cp >= 0xFF01 && cp <= 0xFF60)     // Fullwidth Latin / Punctuation
         || (cp >= 0xFF65 && cp <= 0xFFEF)     // Halfwidth Katakana / Hangul
         || (cp >= 0x20000 && cp <= 0x2A6DF)   // CJK Extension B
         || (cp >= 0x2A700 && cp <= 0x2B73F);  // CJK Extension C
}

// Broader than utf8IsCjkBreakable: this is used only to select a CJK-capable
// UI fallback, not to create line-break opportunities.
inline bool utf8IsCjkCodepoint(const uint32_t cp) {
  return (cp >= 0x1100 && cp <= 0x11FF) || (cp >= 0x2E80 && cp <= 0x2FDF) ||
         (cp >= 0x3000 && cp <= 0x33FF) || (cp >= 0x3400 && cp <= 0x4DBF) ||
         (cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0xA960 && cp <= 0xA97F) ||
         (cp >= 0xAC00 && cp <= 0xD7FF) || (cp >= 0xF900 && cp <= 0xFAFF) ||
         (cp >= 0xFE10 && cp <= 0xFE1F) || (cp >= 0xFE30 && cp <= 0xFE4F) ||
         (cp >= 0xFF01 && cp <= 0xFF60) || (cp >= 0xFF65 && cp <= 0xFFEF) ||
         (cp >= 0x20000 && cp <= 0x2EBEF) || (cp >= 0x2F800 && cp <= 0x2FA1F) ||
         (cp >= 0x30000 && cp <= 0x323AF);
}

// Returns true for Unicode combining diacritical marks that should not advance the cursor.
// Thai vowels that hang BELOW the consonant (sara u, sara uu, phinthu).
// Rendered at font-native vertical position - never raised.
inline bool utf8IsThaiLowerCombiningMark(const uint32_t cp) {
  return cp >= 0x0E38 && cp <= 0x0E3A;
}

// Thai vowels/tone marks/misc that sit ABOVE the consonant.
inline bool utf8IsThaiUpperCombiningMark(const uint32_t cp) {
  return cp == 0x0E31
      || (cp >= 0x0E34 && cp <= 0x0E37)
      || (cp >= 0x0E47 && cp <= 0x0E4E);
}

// Level 2: sits directly on the consonant.
inline bool utf8IsThaiUpperLevelTwoMark(const uint32_t cp) {
  return cp == 0x0E31 || (cp >= 0x0E34 && cp <= 0x0E37) || cp == 0x0E47 || cp == 0x0E4D;
}

// Level 3: stacks ABOVE whatever is already placed.
inline bool utf8IsThaiUpperLevelThreeMark(const uint32_t cp) {
  return (cp >= 0x0E48 && cp <= 0x0E4C) || cp == 0x0E4E;
}

inline bool utf8IsThaiCombiningMark(const uint32_t cp) {
  return utf8IsThaiLowerCombiningMark(cp) || utf8IsThaiUpperCombiningMark(cp);
}

// Returns true for Unicode combining diacritical marks that should not advance the cursor.
inline bool utf8IsCombiningMark(const uint32_t cp) {
  return (cp >= 0x0300 && cp <= 0x036F)
      || (cp >= 0x1DC0 && cp <= 0x1DFF)
      || (cp >= 0x20D0 && cp <= 0x20FF)
      || (cp >= 0xFE20 && cp <= 0xFE2F)
      || utf8IsThaiCombiningMark(cp);
}

// Adjusts *raiseBy so a Thai "level 3" mark (tone marks etc.) stacks ABOVE a
// "level 2" mark (vowel) already placed on the same base glyph. No-op for
// non-Thai marks or when there's nothing to stack on. Caller must reset
// *hasStackedUpper = false whenever a new base (non-combining) glyph is processed.
inline void thaiUpperMarkStack(const uint32_t cp, const int glyphTop, const int glyphHeight, const int penY,
                                int* raiseBy, int* stackedUpperMinY, bool* hasStackedUpper) {
  if (!utf8IsThaiUpperCombiningMark(cp)) return;
  int glyphMinY = penY - *raiseBy + glyphTop - glyphHeight;
  int glyphMaxY = penY - *raiseBy + glyphTop;

  if (utf8IsThaiUpperLevelThreeMark(cp) && *hasStackedUpper) {
    constexpr int MIN_STACK_GAP_PX = 1;
    const int desiredMaxY = *stackedUpperMinY - MIN_STACK_GAP_PX;
    if (glyphMaxY > desiredMaxY) {
      const int extraRaise = glyphMaxY - desiredMaxY;
      *raiseBy += extraRaise;
      glyphMinY -= extraRaise;
      glyphMaxY -= extraRaise;
    }
  }

  if (utf8IsThaiUpperLevelTwoMark(cp) || utf8IsThaiUpperLevelThreeMark(cp)) {
    *stackedUpperMinY = (*hasStackedUpper && *stackedUpperMinY < glyphMinY) ? *stackedUpperMinY : glyphMinY;
    *hasStackedUpper = true;
  }
}
