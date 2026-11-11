// The keyboard model of a skill-flow chart screen.
//
// The feat chart of character generation and the Force-power chart of
// level-up are the same engine structure: CSWGuiSkillFlow rows of up to three
// cells (a chain: base, improved, master), a CSWGuiSkillFlowChart that holds
// the selection, a name label, a description list box and three buttons. The
// generic list-box handler does not fit them - the chart is not row-shaped -
// so they share this one: a 2D cursor over the filled cells, with the buttons
// as one-cell rows below the chart.
//
//   Up / Down     the row; keeps the column, or takes the nearest filled one
//   Left / Right  the filled cells of the row
//   Home / End    the first cell of the chart / the last button
//   Enter         picks the cell (add or remove), or activates the button
//   Esc           the back button
//
// Moving onto a cell selects it in the engine: SetSelectedSkill on the chart
// for the highlight, and the panel's own OnEnter, which fills the name label
// and the description. Those are then spoken, with the cell's status.

#pragma once

#include "menus_skillflow_nav.h"
#include "strings.h"

namespace acc::menus::skillflow_grid {

struct Button {
    void* (*resolve)(void* panel);
    const char* logTag;                 // for the log only
    acc::strings::Id spokenFallback;    // when the button's own text cannot be read
};

// What differs between the screens.
struct Screen {
    const char* logTag;

    // The engine's array of CSWGuiSkillFlow* behind this panel. False on a
    // failed read.
    bool (*readRows)(void* panel, void** rows, int* count);

    void* (*chart)(void* panel);                // the CSWGuiSkillFlowChart
    void* (*nameLabel)(void* panel);
    void* (*descriptionListBox)(void* panel);

    // Panel methods. They hold the engine address themselves, as a constant
    // in the cast, so that tools/analysis/call_convention.py can check it.
    void (*onEnter)(void* panel, unsigned long id);     // refresh name and description
    void (*onPicked)(void* panel, unsigned long id);    // add or remove the pick

    acc::strings::Id unnamedFormat;     // "%u": the name when the label is empty

    const Button* buttons;              // below the chart, in the order Down steps
    int buttonCount;
    void* (*backButton)(void* panel);   // Esc
};

class Grid {
public:
    explicit Grid(const Screen& screen) : screen_(screen) {}

    // True when the event is consumed; `outRv` is then the input hook's result.
    bool HandleInput(void* panel, int code, int value, int& outRv);

private:
    struct Row {
        void*          flow;                            // CSWGuiSkillFlow*
        int            index;                           // in the engine's array, for the log
        unsigned short id[skillflow_nav::kColumns];
    };
    static constexpr int kMaxRows = 256;

    void Bind(void* panel);
    void ReadRows(void* rows, int count);
    int  Total() const { return rowCount_ + screen_.buttonCount; }
    bool OnButton() const { return row_ >= rowCount_; }
    unsigned short CurrentId() const;
    void SelectInEngine(void* panel, unsigned short id);
    void Announce(void* panel);
    void Move(void* panel, const char* key, int row, int col);
    void Activate(void* panel);

    const Screen& screen_;
    Row  rows_[kMaxRows] = {};
    int  rowCount_ = 0;
    int  row_ = 0;      // chart rows first, then one row per button
    int  col_ = 0;      // 0 on a button

    // What the rows were read from. Auto-level-up reuses the panel for the
    // next character with a fresh array: the cursor starts over only then.
    void* boundPanel_ = nullptr;
    void* boundRows_ = nullptr;
    int   boundCount_ = 0;
};

}  // namespace acc::menus::skillflow_grid
