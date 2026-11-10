// Finding a panel's buttons by what they say (Close, Cancel) and the arrow
// buttons next to a control.

#include "menus_chain_internal.h"

#include "engine_offsets.h"
#include "menus_extract.h"

namespace acc::menus::chain {

namespace {

// Does `text` OPEN WITH the whole word `word`? A plain prefix test is not
// enough: the difficulty cycler on the gameplay settings reads "Normal",
// which starts with the cancel keyword "No", so Esc on that screen
// activated the difficulty cycler instead of closing, and since Esc was
// consumed, the screen could not be left by keyboard.
//
// Requiring the next character to be a non-letter/non-digit keeps the
// keyword list a word list rather than a prefix list; trailing punctuation
// still matches.
bool LabelOpensWithWord(const char* text, const char* word) {
    const size_t n = strlen(word);
    if (strncmp(text, word, n) != 0) return false;
    const unsigned char next = static_cast<unsigned char>(text[n]);
    return !(next >= 'A' && next <= 'Z') &&
           !(next >= 'a' && next <= 'z') &&
           !(next >= '0' && next <= '9');
}

// Shared scan behind both finders below: first chain-navigable control whose
// caption opens with one of `words`.
void* FindButtonByWords(void* panel, const char* const* words, int wordCount) {
    if (!panel) return nullptr;
    auto* list = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(panel) + kPanelControlsOffset);
    if (!list->data || list->size <= 0) return nullptr;
    int n = list->size > 256 ? 256 : list->size;
    for (int i = 0; i < n; ++i) {
        void* c = list->data[i];
        if (!IsChainNavigable(c)) continue;
        char text[256];
        if (!acc::menus::extract::FromControl(c, text, sizeof(text), panel)) continue;
        for (int w = 0; w < wordCount; ++w) {
            if (LabelOpensWithWord(text, words[w])) return c;
        }
    }
    return nullptr;
}

}  // namespace

// The nearest text-less control (an arrow button) on the same row as
// `focused`, to its right or left.
void* FindAdjacentArrow(void* panel, void* focused, bool toRight) {
    if (!panel || !focused) return nullptr;

    auto* list = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(panel) + kPanelControlsOffset);
    if (!list->data || list->size <= 0) return nullptr;

    int focusCx, focusCy;
    if (!GetControlCenter(focused, focusCx, focusCy)) return nullptr;

    void* best = nullptr;
    int bestDx = 0x7fffffff;

    // Same-row tolerance. The arrows are centred on their value button, so
    // this only has to absorb rounding — but on KOTOR 2 the rounding is
    // multiplied by the GUI stretch along with everything else.
    const int kSameRowDy = ScaleGuiThresholdPx(5);

    int n = list->size > 256 ? 256 : list->size;
    for (int i = 0; i < n; ++i) {
        void* c = list->data[i];
        if (!c || c == focused) continue;
        if (!IsChainNavigable(c)) continue;

        int cx, cy;
        if (!GetControlCenter(c, cx, cy)) continue;
        if (cy - focusCy > kSameRowDy || focusCy - cy > kSameRowDy) continue;

        int dx = toRight ? (cx - focusCx) : (focusCx - cx);
        if (dx <= 0) continue;

        char tmp[64];
        if (acc::menus::extract::FromControl(c, tmp, sizeof(tmp), panel)) continue;

        if (dx < bestDx) {
            bestDx = dx;
            best   = c;
        }
    }
    return best;
}

void* FindCloseButton(void* panel) {
    static const char* const kWords[] = {"Close", "OK", "Continue"};
    return FindButtonByWords(panel, kWords,
                             sizeof(kWords) / sizeof(kWords[0]));
}

void* FindCancelButton(void* panel) {
    static const char* const kWords[] = {"Cancel", "No"};
    return FindButtonByWords(panel, kWords,
                             sizeof(kWords) / sizeof(kWords[0]));
}

}  // namespace acc::menus::chain
