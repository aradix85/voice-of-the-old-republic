#include "menus_skillflow_grid.h"

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "engine_input.h"
#include "engine_offsets.h"
#include "engine_reads.h"
#include "log.h"
#include "menus_internal.h"   // QueueControlActivate
#include "menus_pending.h"
#include "speech.h"

namespace acc::menus::skillflow_grid {

namespace {

using acc::engine::TryReadPtr;
using acc::engine::TryReadU32;
using skillflow_nav::kColumns;
using skillflow_nav::kEmptyCell;

static_assert(kColumns == kSkillFlowColumnsPerRow, "the chart has three cells a row");

void* CellOf(void* flow, int col) {
    return static_cast<unsigned char*>(flow) + kSkillFlowFirstColumnOffset +
           static_cast<size_t>(col) * kSkillFlowColumnStride;
}

unsigned short ReadCellId(void* flow, int col) {
    uint32_t id = 0;
    if (!TryReadU32(CellOf(flow, col), kFlowSkillStructFeatIdOffset, &id)) return kEmptyCell;
    return id == kFlowSkillStructEmptyFeatId ? kEmptyCell : static_cast<unsigned short>(id);
}

// The chart's render status of a cell, the lowest byte of its status dword:
// 0 available, 1 already learned, 2 granted, 3 locked, 4 chosen.
const char* ReadStatusWord(void* flow, int col, unsigned* outStatus) {
    using acc::strings::Get;
    using acc::strings::Id;
    uint32_t raw = 0;
    *outStatus = TryReadU32(CellOf(flow, col), kFlowSkillStructStatusOffset, &raw)
                     ? (raw & 0xffu) : 0xffu;
    switch (*outStatus) {
        case 0:  return Get(Id::ChargenFeatStatusAvailable);
        case 1:  return Get(Id::ChargenFeatStatusExisting);
        case 2:  return Get(Id::ChargenFeatStatusGranted);
        case 3:  return Get(Id::ChargenFeatStatusLocked);
        case 4:  return Get(Id::ChargenFeatStatusChosen);
        default: return "";
    }
}

// The rendered text of the description list box's first row, which the
// panel's OnEnter has just filled.
bool ReadDescription(void* listBox, char* out, size_t n) {
    out[0] = '\0';
    void* first = acc::engine::ReadListBoxRow(listBox, 0);
    return first && acc::engine::ReadLabelText(first, out, n);
}

}  // namespace

unsigned short Grid::CurrentId() const {
    return OnButton() ? kEmptyCell : rows_[row_].id[col_];
}

// Rows without a single cell are left out.
void Grid::ReadRows(void* rows, int count) {
    rowCount_ = 0;
    if (!rows || count < 0 || count > kMaxRows) return;
    for (int r = 0; r < count; ++r) {
        void* flow = nullptr;
        if (!TryReadPtr(rows, static_cast<size_t>(r) * sizeof(void*), &flow) || !flow) continue;
        Row& row = rows_[rowCount_];
        row.flow  = flow;
        row.index = r;
        for (int c = 0; c < kColumns; ++c) row.id[c] = ReadCellId(flow, c);
        if (skillflow_nav::FirstFilledCol(row.id) >= 0) ++rowCount_;
    }
}

// The rows are read again on every event, so that a row pointer is never
// stale. The cursor keeps its place unless the panel was bound anew.
void Grid::Bind(void* panel) {
    void* rows = nullptr;
    int count = 0;
    screen_.readRows(panel, &rows, &count);
    const bool rebound = panel != boundPanel_ || rows != boundRows_ || count != boundCount_;
    boundPanel_ = panel;
    boundRows_  = rows;
    boundCount_ = count;
    ReadRows(rows, count);

    if (!rebound) {
        row_ = (std::max)(0, (std::min)(row_, Total() - 1));
        if (OnButton()) col_ = 0;
        return;
    }
    row_ = 0;
    col_ = rowCount_ > 0 ? skillflow_nav::FirstFilledCol(rows_[0].id) : 0;
    acclog::Write(screen_.logTag, "bound panel=%p chartRows=%d totalRows=%d cursor=(r=%d, c=%d)",
                  panel, rowCount_, Total(), row_, col_);
}

void Grid::SelectInEngine(void* panel, unsigned short id) {
    typedef void (__thiscall* PFN_SetSelectedSkill)(void* chart, unsigned long id);
    __try {
        reinterpret_cast<PFN_SetSelectedSkill>(kAddrCSWGuiSkillFlowChartSetSelectedSkill)(
            screen_.chart(panel), id);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write(screen_.logTag, "SetSelectedSkill faulted id=%u", static_cast<unsigned>(id));
    }
    __try {
        screen_.onEnter(panel, id);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write(screen_.logTag, "OnEnter faulted id=%u", static_cast<unsigned>(id));
    }
}

void Grid::Announce(void* panel) {
    if (OnButton()) {
        const Button& button = screen_.buttons[row_ - rowCount_];
        void* target = button.resolve(panel);
        char text[64] = "";
        if (!target || !acc::engine::ReadButtonText(target, text, sizeof(text)) || !text[0]) {
            strncpy_s(text, acc::strings::Get(button.spokenFallback), _TRUNCATE);
        }
        acc::speech::Speak(text, /*interrupt=*/false);
        acclog::Write(screen_.logTag, "focus button %s target=%p text=\"%s\"",
                      button.logTag, target, text);
        return;
    }

    const unsigned short id = CurrentId();
    if (id == kEmptyCell) return;
    SelectInEngine(panel, id);

    char name[128] = "";
    if (!acc::engine::ReadLabelText(screen_.nameLabel(panel), name, sizeof(name)) || !name[0]) {
        std::snprintf(name, sizeof(name), acc::strings::Get(screen_.unnamedFormat),
                      static_cast<unsigned>(id));
    }
    unsigned status = 0;
    const char* statusWord = ReadStatusWord(rows_[row_].flow, col_, &status);

    char cell[256];
    std::snprintf(cell, sizeof(cell),
                  acc::strings::Get(acc::strings::Id::FmtChargenFeatChartCell), name, statusWord);
    // A chain reads as one line of a list unless the cells to its right are named.
    char head[320];
    const int moreRight = skillflow_nav::FilledBeyond(rows_[row_].id, col_, +1);
    if (moreRight > 0) {
        std::snprintf(head, sizeof(head),
                      acc::strings::Get(acc::strings::Id::FmtChargenFeatChartMoreRight),
                      cell, moreRight);
    } else {
        strncpy_s(head, cell, _TRUNCATE);
    }
    acc::speech::Speak(head, /*interrupt=*/false);

    char description[1024];
    if (ReadDescription(screen_.descriptionListBox(panel), description, sizeof(description)) &&
        description[0]) {
        acc::speech::Speak(description, /*interrupt=*/false);
    }
    acclog::Write(screen_.logTag, "focus row=%d col=%d id=%u status=%u name=\"%s\"",
                  rows_[row_].index, col_, static_cast<unsigned>(id), status, name);
}

// Announces also when the cursor could not move: the repeat says that this
// is the edge.
void Grid::Move(void* panel, const char* key, int row, int col) {
    const int oldRow = row_;
    const int oldCol = col_;
    row_ = row;
    col_ = (std::max)(0, col);
    Announce(panel);
    acclog::Write(screen_.logTag, "%s (r=%d,c=%d) -> (r=%d,c=%d) [chartRows=%d, totalRows=%d]",
                  key, oldRow, oldCol, row_, col_, rowCount_, Total());
}

void Grid::Activate(void* panel) {
    if (OnButton()) {
        detail::QueueControlActivate(screen_.buttons[row_ - rowCount_].resolve(panel),
                                     "SkillFlowGrid: Enter -> button");
        return;
    }
    const unsigned short id = CurrentId();
    if (id == kEmptyCell) return;
    if (pending::IsPending()) {
        acclog::Write(screen_.logTag, "Enter: an activation is already pending; ignored");
        return;
    }
    __try {
        screen_.onPicked(panel, id);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write(screen_.logTag, "OnPicked faulted id=%u", static_cast<unsigned>(id));
    }
    acclog::Write(screen_.logTag, "picked panel=%p id=%u", panel, static_cast<unsigned>(id));
    // The status flips on add and remove. For a cell that cannot change, the
    // engine opens a message box instead; its title is spoken on top of this.
    Announce(panel);
}

bool Grid::HandleInput(void* panel, int code, int value, int& outRv) {
    using namespace acc::engine;  // kInput*
    Bind(panel);
    if (value == 0) return false;  // press edges only

    const int last = Total() - 1;
    const auto columnIn = [this](int row, int want) {
        return row >= rowCount_ ? 0 : skillflow_nav::NearestFilledCol(rows_[row].id, want);
    };
    if (code == kInputNavUp || code == kInputNavDown) {
        const bool down = code == kInputNavDown;
        const int row = (std::max)(0, (std::min)(row_ + (down ? 1 : -1), last));
        Move(panel, down ? "Down" : "Up", row, columnIn(row, col_));
    } else if (code == kInputNavLeft || code == kInputNavRight) {
        const bool right = code == kInputNavRight;
        const int col = OnButton() ? 0 : skillflow_nav::StepCol(rows_[row_].id, col_,
                                                                right ? 1 : -1);
        Move(panel, right ? "Right" : "Left", row_, col);
    } else if (code == kInputHome) {
        Move(panel, "Home", 0, columnIn(0, 0));
    } else if (code == kInputEnd) {
        Move(panel, "End", last, 0);
    } else if (code == kInputEnter1 || code == kInputEnter2) {
        Activate(panel);
    } else if (code == kInputEsc1 || code == kInputEsc2) {
        detail::QueueControlActivate(screen_.backButton(panel), "SkillFlowGrid: Esc -> back");
    } else {
        return false;
    }
    outRv = 1;
    return true;
}

}  // namespace acc::menus::skillflow_grid
