// The rows of the examine panel, built from the creature reads.

#include "examine_view_internal.h"

#include "combat_query.h"    // ReadEquippedItemName
#include "engine_area.h"
#include "engine_offsets.h"
#include "engine_reads.h"     // ReadObjectCurrentHitPoints

#include <windows.h>

#include <cstdarg>
#include <cstdio>

namespace acc::examine_view {

namespace {

using S = acc::strings::Id;

// Appends one formatted row while there is room.
void AddRow(int& idx, const char* fmt, ...) {
    if (idx >= kMaxRows) return;
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(g_state.rows[idx], sizeof(g_state.rows[0]), fmt, ap);
    va_end(ap);
    ++idx;
}

void AddText(int& idx, S text) {
    AddRow(idx, "%s", acc::strings::Get(text));
}

// One row per effect of the object; an unmapped type gets its number.
void AppendEffectRows(void* serverObject, int& idx) {
    int types[64];
    const int n = acc::engine::ReadObjectEffectTypes(serverObject, types, 64);
    for (int i = 0; i < n; ++i) {
        if (const char* name = EffectName(types[i])) {
            AddRow(idx, acc::strings::Get(S::FmtExamineRowEffect), name);
        } else {
            AddRow(idx, acc::strings::Get(S::FmtExamineRowEffectUnknown), types[i]);
        }
    }
}

// One row per feat in CSWSCreatureStats.feats (CExoArrayList<ushort>).
void AppendFeatRows(void* serverCreature, int& idx) {
    void* stats = ReadCreatureStats(serverCreature);
    if (!stats) return;
    __try {
        auto* lst = reinterpret_cast<CExoArrayList*>(
            reinterpret_cast<unsigned char*>(stats) + kStatsFeatsListOffset);
        if (!lst || !lst->data || lst->size <= 0) return;
        const int n = lst->size > 0x100 ? 0x100 : lst->size;
        auto* ids = reinterpret_cast<unsigned short*>(lst->data);
        for (int i = 0; i < n && idx < kMaxRows; ++i) {
            const unsigned short featId = ids[i];
            char nameBuf[128] = "";
            if (ResolveFeatName(featId, nameBuf, sizeof(nameBuf)) && nameBuf[0] != '\0') {
                AddRow(idx, acc::strings::Get(S::FmtExamineRowFeat), nameBuf);
            } else {
                AddRow(idx, acc::strings::Get(S::FmtExamineRowFeatUnknown),
                       static_cast<int>(featId));
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

bool EquippedName(void* creature, size_t slotOffset, char* out, size_t cap) {
    return acc::combat::query::ReadEquippedItemName(creature, slotOffset, "examine",
                                                    out, cap) && out[0] != '\0';
}

// An occupied equipment slot gets a row; an empty one none (a sighted player
// sees what is worn, not a list of empty slots).
void AddEquipRow(void* creature, int& idx, size_t slotOffset, S fmt) {
    char item[96] = "";
    if (EquippedName(creature, slotOffset, item, sizeof(item))) {
        AddRow(idx, acc::strings::Get(fmt), item);
    }
}

void AddCreatureStatRows(void* obj, int& idx) {
    AddRow(idx, acc::strings::Get(S::FmtExamineRowFaction),
           acc::strings::Get(FactionWordIdFor(ReadFactionId(obj))));
    const int dl = ReadDamageLevel(obj);
    if (dl >= 0) {
        AddRow(idx, acc::strings::Get(S::FmtExamineRowCondition),
               acc::strings::Get(acc::combat::query::DamageLevelStringId(dl)));
    }
    const int hpCur = acc::engine::ReadObjectCurrentHitPoints(obj);
    const int hpMax = ReadHpMax(obj);
    if (hpCur >= 0 && hpMax > 0) {
        AddRow(idx, acc::strings::Get(S::FmtExamineRowHpFull), hpCur, hpMax);
    } else if (hpCur >= 0) {
        AddRow(idx, acc::strings::Get(S::FmtExamineRowHp), hpCur);
    }
    const int lvl = ReadLevel(obj);
    if (lvl > 0) AddRow(idx, acc::strings::Get(S::FmtExamineRowLevel), lvl);
}

void AddCreatureDetailRows(void* obj, int& idx) {
    // Flags only when set; invisibility shows up among the effects.
    if (ReadBlindFlag(obj)) AddText(idx, S::ExamineRowStatusBlind);

    char wpn[96] = "";
    if (EquippedName(obj, kInventoryRightWeaponHandleOffset, wpn, sizeof(wpn))) {
        AddRow(idx, acc::strings::Get(S::FmtExamineRowWeapon), wpn);
    } else {
        AddText(idx, S::ExamineRowWeaponNone);
    }
    AddEquipRow(obj, idx, kInventoryLeftWeaponHandleOffset, S::FmtExamineRowOffHand);
    AddEquipRow(obj, idx, kInventoryHeadHandleOffset,       S::FmtExamineRowHead);
    AddEquipRow(obj, idx, kInventoryTorsoHandleOffset,      S::FmtExamineRowTorso);
    AddEquipRow(obj, idx, kInventoryHandsHandleOffset,      S::FmtExamineRowHands);

    // "No effects" / "No feats" confirm an empty list rather than leave it out.
    const int effectStart = idx;
    AppendEffectRows(obj, idx);
    if (idx == effectStart) AddText(idx, S::ExamineRowNoEffects);
    const int featStart = idx;
    AppendFeatRows(obj, idx);
    if (idx == featStart) AddText(idx, S::ExamineRowNoFeats);
}

}  // namespace

int BuildRows() {
    void* obj = nullptr;
    if (!IsSentinel(g_state.targetHandle)) {
        obj = acc::engine::ResolveClientObjectHandle(g_state.targetHandle);
        if (!obj) obj = acc::engine::ResolveServerObjectHandle(g_state.targetHandle);
    }
    g_state.targetObj = obj;
    if (!obj) return 0;

    const bool isCreature =
        acc::engine::GetObjectKind(obj) == static_cast<int>(acc::engine::GameObjectKind::Creature);
    int idx = 0;

    char name[96] = "";
    if (!acc::engine::GetObjectDisplayNameByHandle(g_state.targetHandle, name, sizeof(name)) ||
        name[0] == '\0') {
        acc::engine::GetObjectName(obj, name, sizeof(name));
    }
    AddRow(idx, acc::strings::Get(S::FmtExamineRowName), name[0] ? name : "?");

    if (isCreature) AddCreatureStatRows(obj, idx);
    const int dist = Read2DDistanceMeters(obj);
    if (dist >= 0) AddRow(idx, acc::strings::Get(S::FmtExamineRowDistance), dist);
    if (isCreature) AddCreatureDetailRows(obj, idx);
    return idx;
}

}  // namespace acc::examine_view
