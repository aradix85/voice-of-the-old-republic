// The chargen class selection: recognising a class icon, and the class name and description cache
// that makes each icon speakable.

#include "menus_internal_internal.h"

#include "strings.h"
#include "engine_offsets.h"
#include "engine_reads.h"

namespace acc::menus::detail {

namespace {

// Per-icon class-name cache for CSWGuiClassSelection. See the long
// comment in ExtractAnnounceableText step 9c for why this exists. Sized
// to hold all 6 icons of a single panel; key is (panel, icon). Keyed by
// panel as well as icon so a chargen restart on a new panel instance
// doesn't surface stale entries from the previous run. First-write
// wins — once an entry is locked, subsequent updates are ignored so the
// engine's transient class_label revert can't corrupt a settled value.
// `text` is the composed announce: the class name off class_label, plus the
// class blurb off LBL_DESC when the panel had one. Sized for both — the
// German blurbs run to about 200 characters.
struct ClassLabelCacheEntry {
    void* panel;
    void* icon;
    char  text[acc::menus::detail::kAnnounceTextMax];
};
static constexpr int kClassLabelCacheSize = 8;
static ClassLabelCacheEntry g_classLabelCache[kClassLabelCacheSize];

// The class-description label's authored text, snapshotted at panel-walk
// time. classsel.gui ships LBL_DESC with a placeholder — empty on KOTOR 1,
// the literal "This is just a test line." repeated on KOTOR 2 — and the
// engine overwrites it with the real class description on hover. Comparing
// against the snapshot is how we tell "the engine has written something" from
// "this is still the .gui default", without hard-coding either placeholder.
// One slot: only one class-selection panel exists at a time.
static void* g_classDescBaselinePanel = nullptr;
static char  g_classDescBaseline[384] = {0};

}  // namespace

// Positional detector for chargen class-icon buttons. The 6 class icons
// are CSWGuiClassSelChar[6] starting at panel+0x6c with stride 0x25c, and
// each CSWGuiClassSelChar embeds a CSWGuiButton at offset 0 — so a focused
// class-icon control pointer lands exactly on `panel + 0x6c + i * 0x25c`
// for some 0 ≤ i < 6.
bool IsClassSelectionIcon(void* panel, void* control) {
    if (!panel || !control) return false;
    // Guarded deref: this is the gate for menus_extract's step 9c, so it
    // runs for every control that reaches that far on EVERY panel, not just
    // chargen — and its caller's `panel` has only cleared IsPanelInManager,
    // which proves the pointer is listed, not that the object is alive.
    if (!acc::engine::HasVtable(panel, kVtableCSWGuiClassSelection)) {
        return false;
    }
    auto* panelBase = reinterpret_cast<unsigned char*>(panel);
    auto* ctrlBase  = reinterpret_cast<unsigned char*>(control);
    ptrdiff_t off = ctrlBase - panelBase;
    ptrdiff_t arrayEnd = (ptrdiff_t)(kClassSelectionsArrayOffset +
                                     kClassSelectionsCount * kClassSelCharSize);
    if (off < (ptrdiff_t)kClassSelectionsArrayOffset || off >= arrayEnd) {
        return false;
    }
    return ((off - (ptrdiff_t)kClassSelectionsArrayOffset) %
            (ptrdiff_t)kClassSelCharSize) == 0;
}

void ClassDescBaselineCapture(void* panel, const char* text) {
    if (!panel) return;
    g_classDescBaselinePanel = panel;
    if (text) {
        strncpy_s(g_classDescBaseline, text, _TRUNCATE);
    } else {
        g_classDescBaseline[0] = '\0';
    }
}

bool ClassDescIsEngineWritten(void* panel, const char* text) {
    if (!text || text[0] == '\0') return false;
    // No snapshot for this panel: we never saw its authored state, so we
    // cannot tell content from placeholder. Stay quiet rather than risk
    // speaking "This is just a test line." at the user.
    if (panel != g_classDescBaselinePanel) return false;
    return strcmp(text, g_classDescBaseline) != 0;
}

const char* ClassLabelCacheLookup(void* panel, void* icon) {
    for (int i = 0; i < kClassLabelCacheSize; ++i) {
        const auto& e = g_classLabelCache[i];
        if (e.panel == panel && e.icon == icon && e.text[0] != '\0') {
            return e.text;
        }
    }
    return nullptr;
}

// Compose what the user hears for one class icon: the class name, followed by
// the blurb the engine renders beneath it, when there is one. Both call sites
// (the extractor's cache-fill and the outgoing-icon prefill) go through here
// so the cached string is identical whichever path wins the race.
//
// Reading LBL_DESC has to happen at the same instant class_label is read —
// by the time a chain step announces an icon, both labels already hold the
// NEXT icon's text.
void ComposeClassAnnounce(void* panel, const char* name,
                                              char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize < 2) return;
    outBuf[0] = '\0';
    if (!name || name[0] == '\0') return;
    strncpy_s(outBuf, bufSize, name, _TRUNCATE);
    if (!panel) return;

    void* descLabel = acc::menus::detail::FindControlById(panel,
                                                          kClassSelDescLabelId);
    if (!descLabel) return;
    char desc[acc::menus::detail::kAnnounceTextMax] = {0};
    if (!acc::engine::ReadLabelText(descLabel, desc, sizeof(desc))) return;
    if (!ClassDescIsEngineWritten(panel, desc)) return;

    char composed[acc::menus::detail::kAnnounceTextMax];
    snprintf(composed, sizeof(composed),
             acc::strings::Get(acc::strings::Id::FmtClassNameWithDescription),
             name, desc);
    strncpy_s(outBuf, bufSize, composed, _TRUNCATE);
}

void ClassLabelCacheStore(void* panel, void* icon, const char* text) {
    if (!panel || !icon || !text || text[0] == '\0') return;
    // First-write wins for a (panel, icon) pair: bail out if already cached.
    for (int i = 0; i < kClassLabelCacheSize; ++i) {
        const auto& e = g_classLabelCache[i];
        if (e.panel == panel && e.icon == icon) return;
    }
    // Find a free slot, evicting any entry from a different panel if full.
    int slot = -1;
    for (int i = 0; i < kClassLabelCacheSize; ++i) {
        if (g_classLabelCache[i].panel == nullptr) { slot = i; break; }
    }
    if (slot < 0) {
        for (int i = 0; i < kClassLabelCacheSize; ++i) {
            if (g_classLabelCache[i].panel != panel) { slot = i; break; }
        }
    }
    if (slot < 0) return;
    g_classLabelCache[slot].panel = panel;
    g_classLabelCache[slot].icon  = icon;
    strncpy_s(g_classLabelCache[slot].text, text, _TRUNCATE);
}

}  // namespace acc::menus::detail
