// The rows of the examine panel, built from the creature reads.

#include "examine_view_internal.h"

#include "engine_area.h"
#include "engine_offsets.h"
#include "engine_reads.h"   // LookupTlk — strref → localized text, both games

namespace acc::examine_view {

namespace {

// Walk CSWSObject.effects (CExoArrayList<CGameEffect*> @+0x124) and emit
// rows for up to `cap` effects. Returns the number of rows actually
// written (could be < count if buffer is full).
int AppendEffectRows(void* serverObject, char rows[][192],
                     int& outIdx, int rowCap) {
    if (!serverObject) return 0;
    int wrote = 0;
    __try {
        auto* lst = reinterpret_cast<CExoArrayList*>(
            reinterpret_cast<unsigned char*>(serverObject) +
            kObjectEffectsOffset);
        if (!lst || !lst->data || lst->size <= 0) return 0;
        int n = lst->size > 64 ? 64 : lst->size;
        for (int i = 0; i < n && outIdx < rowCap; ++i) {
            void* eff = lst->data[i];
            if (!eff) continue;
            int type = static_cast<int>(*reinterpret_cast<unsigned short*>(
                reinterpret_cast<unsigned char*>(eff) +
                kGameEffectTypeOffset));
            const char* name = EffectName(type);
            if (name) {
                std::snprintf(rows[outIdx], sizeof(rows[0]),
                              acc::strings::Get(
                                  acc::strings::Id::FmtExamineRowEffect),
                              name);
            } else {
                std::snprintf(rows[outIdx], sizeof(rows[0]),
                              acc::strings::Get(
                                  acc::strings::Id::FmtExamineRowEffectUnknown),
                              type);
            }
            ++outIdx;
            ++wrote;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // Silent — partial data is fine.
    }
    return wrote;
}

// Walk CSWSCreatureStats.feats (CExoArrayList<ushort> @+0x0) and emit
// rows for up to `cap` feats with localized names from CSWRules::GetFeat.
int AppendFeatRows(void* serverCreature, char rows[][192],
                   int& outIdx, int rowCap) {
    void* stats = ReadCreatureStats(serverCreature);
    if (!stats) return 0;
    int wrote = 0;
    __try {
        auto* lst = reinterpret_cast<CExoArrayList*>(
            reinterpret_cast<unsigned char*>(stats) +
            kStatsFeatsListOffset);
        if (!lst || !lst->data || lst->size <= 0) return 0;
        int n = lst->size > 0x100 ? 0x100 : lst->size;
        auto* ids = reinterpret_cast<unsigned short*>(lst->data);
        for (int i = 0; i < n && outIdx < rowCap; ++i) {
            unsigned short featId = ids[i];
            char nameBuf[128] = "";
            if (ResolveFeatName(featId, nameBuf, sizeof(nameBuf)) &&
                nameBuf[0] != '\0') {
                std::snprintf(rows[outIdx], sizeof(rows[0]),
                              acc::strings::Get(
                                  acc::strings::Id::FmtExamineRowFeat),
                              nameBuf);
            } else {
                std::snprintf(rows[outIdx], sizeof(rows[0]),
                              acc::strings::Get(
                                  acc::strings::Id::FmtExamineRowFeatUnknown),
                              (int)featId);
            }
            ++outIdx;
            ++wrote;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // Partial is fine.
    }
    return wrote;
}

}  // namespace

// Row building.

// (Re)build the row list for the cached target. Returns row count
// (0 if target unresolvable, view should disarm).
int BuildRows() {
    using S = acc::strings::Id;

    void* obj = nullptr;
    if (!IsSentinel(g_state.targetHandle)) {
        obj = acc::engine::ResolveClientObjectHandle(g_state.targetHandle);
        if (!obj) {
            obj = acc::engine::ResolveServerObjectHandle(
                g_state.targetHandle);
        }
    }
    g_state.targetObj = obj;
    if (!obj) return 0;

    int kind = acc::engine::GetObjectKind(obj);
    int idx = 0;

    // Row: Name.
    char name[96] = "";
    if (!acc::engine::GetObjectDisplayNameByHandle(
            g_state.targetHandle, name, sizeof(name)) || name[0] == '\0') {
        acc::engine::GetObjectName(obj, name, sizeof(name));
    }
    if (name[0] == '\0') std::strncpy(name, "?", sizeof(name) - 1);
    std::snprintf(g_state.rows[idx], sizeof(g_state.rows[0]),
                  acc::strings::Get(S::FmtExamineRowName), name);
    ++idx;

    // Creature-only rows.
    bool isCreature = (kind ==
        static_cast<int>(acc::engine::GameObjectKind::Creature));

    // Small helper to append a single optional equipment-slot row.
    auto appendEquipRow = [&](size_t slotOffset, S fmtId) {
        if (idx >= kMaxRows) return;
        char item[96] = "";
        if (ReadEquippedItemNameAtSlot(obj, slotOffset, item, sizeof(item)) &&
            item[0] != '\0') {
            std::snprintf(g_state.rows[idx], sizeof(g_state.rows[0]),
                          acc::strings::Get(fmtId), item);
            ++idx;
        }
    };

    if (isCreature) {
        // Row: Faction.
        int factionId = ReadFactionId(obj);
        acc::strings::Id factionWord = FactionWordIdFor(factionId);
        std::snprintf(g_state.rows[idx], sizeof(g_state.rows[0]),
                      acc::strings::Get(S::FmtExamineRowFaction),
                      acc::strings::Get(factionWord));
        ++idx;

        // Row: Condition (damage level — visible wound state).
        if (idx < kMaxRows) {
            int dl = ReadDamageLevel(obj);
            if (dl >= 0) {
                std::snprintf(g_state.rows[idx], sizeof(g_state.rows[0]),
                              acc::strings::Get(S::FmtExamineRowCondition),
                              acc::strings::Get(DamageLevelStringId(dl)));
                ++idx;
            }
        }

        // Row: HP — full "cur of max" when GetMaxHitPoints resolves,
        // otherwise the older single-value form.
        if (idx < kMaxRows) {
            int hpCur = acc::engine::ReadObjectCurrentHitPoints(obj);
            int hpMax = ReadHpMax(obj);
            if (hpCur >= 0 && hpMax > 0) {
                std::snprintf(g_state.rows[idx], sizeof(g_state.rows[0]),
                              acc::strings::Get(S::FmtExamineRowHpFull),
                              hpCur, hpMax);
                ++idx;
            } else if (hpCur >= 0) {
                std::snprintf(g_state.rows[idx], sizeof(g_state.rows[0]),
                              acc::strings::Get(S::FmtExamineRowHp), hpCur);
                ++idx;
            }
        }

        // Row: Level.
        if (idx < kMaxRows) {
            int lvl = ReadLevel(obj);
            if (lvl > 0) {
                std::snprintf(g_state.rows[idx], sizeof(g_state.rows[0]),
                              acc::strings::Get(S::FmtExamineRowLevel), lvl);
                ++idx;
            }
        }
    }

    // Row: Distance (always shown for resolvable target).
    int dist = Read2DDistanceMeters(obj);
    if (dist >= 0 && idx < kMaxRows) {
        std::snprintf(g_state.rows[idx], sizeof(g_state.rows[0]),
                      acc::strings::Get(S::FmtExamineRowDistance), dist);
        ++idx;
    }

    if (isCreature) {
        // Status flags — only emit a row when the flag is set, so a
        // healthy normal creature doesn't get a row of "not invisible".
        // (Invisible is covered read-only by the effect list — see the note on
        // ReadBlindFlag; the engine GetInvisible call was removed.)
        if (idx < kMaxRows && ReadBlindFlag(obj)) {
            std::strncpy(g_state.rows[idx],
                         acc::strings::Get(S::ExamineRowStatusBlind),
                         sizeof(g_state.rows[0]) - 1);
            g_state.rows[idx][sizeof(g_state.rows[0]) - 1] = '\0';
            ++idx;
        }

        // Rows: Equipment — main hand, off hand, head, torso, hands.
        // Only emit rows for occupied slots (sighted players just see
        // what's worn, not a list of empty slots).
        if (idx < kMaxRows) {
            char wpn[96] = "";
            if (ReadEquippedItemNameAtSlot(obj,
                    kInventoryRightWeaponHandleOffset,
                    wpn, sizeof(wpn)) && wpn[0] != '\0') {
                std::snprintf(g_state.rows[idx], sizeof(g_state.rows[0]),
                              acc::strings::Get(S::FmtExamineRowWeapon),
                              wpn);
            } else {
                std::strncpy(g_state.rows[idx],
                             acc::strings::Get(S::ExamineRowWeaponNone),
                             sizeof(g_state.rows[0]) - 1);
                g_state.rows[idx][sizeof(g_state.rows[0]) - 1] = '\0';
            }
            ++idx;
        }
        appendEquipRow(kInventoryLeftWeaponHandleOffset, S::FmtExamineRowOffHand);
        appendEquipRow(kInventoryHeadHandleOffset,       S::FmtExamineRowHead);
        appendEquipRow(kInventoryTorsoHandleOffset,      S::FmtExamineRowTorso);
        appendEquipRow(kInventoryHandsHandleOffset,      S::FmtExamineRowHands);

        // Rows: Effects (one per active effect). Append "No effects" if
        // none — gives the user a confirmed-empty signal vs absence.
        int effectStart = idx;
        AppendEffectRows(obj, g_state.rows, idx, kMaxRows);
        if (idx == effectStart && idx < kMaxRows) {
            std::strncpy(g_state.rows[idx],
                         acc::strings::Get(S::ExamineRowNoEffects),
                         sizeof(g_state.rows[0]) - 1);
            g_state.rows[idx][sizeof(g_state.rows[0]) - 1] = '\0';
            ++idx;
        }

        // Rows: Feats.
        int featStart = idx;
        AppendFeatRows(obj, g_state.rows, idx, kMaxRows);
        if (idx == featStart && idx < kMaxRows) {
            std::strncpy(g_state.rows[idx],
                         acc::strings::Get(S::ExamineRowNoFeats),
                         sizeof(g_state.rows[0]) - 1);
            g_state.rows[idx][sizeof(g_state.rows[0]) - 1] = '\0';
            ++idx;
        }
    }

    return idx;
}

}  // namespace acc::examine_view
