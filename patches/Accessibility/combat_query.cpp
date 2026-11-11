// Combat readouts: the brief added to a creature's name when it becomes the
// target, and the leader's own status on H.

#include "combat_query.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>

#include "engine_area.h"      // GetObjectKind, GetObjectPosition, GetObjectDisplayNameByHandle
#include "engine_offsets.h"
#include "engine_panels.h"    // IsForegroundUiBlocking, PanelKindName
#include "engine_player.h"    // GetPlayerServerCreature, GetClientLeader, GetPlayerPosition
#include "engine_reads.h"
#include "examine_view.h"     // EffectName, EffectIconName
#include "hotkeys.h"
#include "log.h"
#include "speech.h"
#include "strfmt.h"
#include "strings.h"

namespace acc::combat::query {

bool ReadEquippedItemName(void* serverCreature, size_t slotHandleOffset,
                          const char* slotLabel,
                          char* outBuf, size_t outBufSize) {
    outBuf[0] = '\0';
    void* inventory = nullptr;
    uint32_t handle = 0;
    if (!acc::engine::TryReadPtr(serverCreature, kCreatureInventoryOffset, &inventory) ||
        !inventory ||
        !acc::engine::TryReadU32(inventory, slotHandleOffset, &handle)) {
        return false;
    }
    if (handle == 0u || handle == 0xFFFFFFFFu || handle == kInvalidObjectId) {
        return false;  // empty slot
    }

    if (!acc::engine::GetObjectDisplayNameByHandle(handle, outBuf, outBufSize) ||
        outBuf[0] == '\0') {
        acclog::Write("Combat.Equip", "slot=%s handle=0x%08x name-resolve missed",
                      slotLabel, handle);
        outBuf[0] = '\0';
        return false;
    }
    // NPC-only creature-prop weapons (K2: "{Prop QS 01}" quarterstaff,
    // "{Prop BC 01}" blaster cannon, ...) carry a literal dev-placeholder
    // name in the template, braces included. Sighted players never see
    // enemy weapon names, so the data was never localized. Treated as an
    // empty slot.
    if (outBuf[0] == '{') {
        acclog::Write("Combat.Equip",
                      "slot=%s handle=0x%08x placeholder name [%s] suppressed",
                      slotLabel, handle, outBuf);
        outBuf[0] = '\0';
        return false;
    }
    return true;
}

namespace {

constexpr int kMaxEffectsSpoken = 5;
constexpr int kMaxEffectsRead   = 64;

// Horizontal distance from the player to the target, -1 when a position is
// unreadable. Height is ignored, as in cycle_state, so a target on another
// floor does not read as far away.
float PlayerDistanceMeters(void* target) {
    Vector tgt{};
    Vector pc{};
    if (!acc::engine::GetObjectPosition(target, tgt) ||
        !acc::engine::GetPlayerPosition(pc)) {
        return -1.0f;
    }
    return DistXY(tgt, pc);
}

// Comma-joins the names of up to kMaxEffectsSpoken distinct ids; an id
// without a name is skipped. True when at least one name was written.
bool JoinEffectNames(const int* ids, int count, const char* (*nameOf)(int),
                     char* outBuf, size_t outBufSize) {
    int spoken[kMaxEffectsSpoken];
    int spokenCount = 0;
    size_t off = 0;
    for (int i = 0; i < count && spokenCount < kMaxEffectsSpoken; ++i) {
        const char* name = nameOf(ids[i]);
        if (!name) continue;
        if (std::find(spoken, spoken + spokenCount, ids[i]) != spoken + spokenCount) {
            continue;
        }
        spoken[spokenCount++] = ids[i];

        const int n = std::snprintf(outBuf + off, outBufSize - off,
                                    (off == 0) ? "%s" : ", %s", name);
        if (n < 0) break;
        off += static_cast<size_t>(n);
        if (off >= outBufSize) break;
    }
    return outBuf[0] != '\0';
}

// The effects clause. First choice is the icon row a sighted player sees on
// the portrait, by name, in display order. A creature with effects but no
// icons (script-applied buffs without an EFFECTICON, such as Malak's fight
// buffs) falls back to its raw effect list, mapped types only; the examine
// view lists the unmapped ones.
bool BuildEffectsSummary(void* serverCreature, char* outBuf, size_t outBufSize) {
    outBuf[0] = '\0';
    int ids[kMaxEffectsRead];
    int n = acc::engine::ReadCreatureEffectIconIds(serverCreature, ids, kMaxEffectsRead);
    if (JoinEffectNames(ids, n, acc::examine_view::EffectIconName, outBuf, outBufSize)) {
        return true;
    }
    n = acc::engine::ReadObjectEffectTypes(serverCreature, ids, kMaxEffectsRead);
    return JoinEffectNames(ids, n, acc::examine_view::EffectName, outBuf, outBufSize);
}

struct Loadout {
    char effects[192] = "";
    char mainWpn[96]  = "";
    char offWpn[96]   = "";
};

// What both readouts end with: the active effects, then what each hand
// holds. An empty hand is skipped rather than announced.
void AppendLoadout(acc::strfmt::FixedText& b, void* serverCreature, Loadout& out) {
    using S = acc::strings::Id;
    if (BuildEffectsSummary(serverCreature, out.effects, sizeof(out.effects))) {
        b.Append(acc::strings::Get(S::FmtBriefEffects), out.effects);
    }
    if (ReadEquippedItemName(serverCreature, kInventoryRightWeaponHandleOffset,
                             "main-hand", out.mainWpn, sizeof(out.mainWpn))) {
        b.Append(acc::strings::Get(S::FmtBriefWielding), out.mainWpn);
    }
    if (ReadEquippedItemName(serverCreature, kInventoryLeftWeaponHandleOffset,
                             "off-hand", out.offWpn, sizeof(out.offWpn))) {
        b.Append(acc::strings::Get(S::FmtBriefOffHand), out.offWpn);
    }
}

// The leader's hit points, Force points for a Force user, effects and
// weapons.
void SpeakSelfStatus() {
    using S = acc::strings::Id;
    void* creature = acc::engine::GetPlayerServerCreature();
    if (!creature) {
        const char* phrase = acc::strings::Get(S::PcStatNoCharacter);
        acc::speech::Speak(phrase, /*interrupt=*/true);
        acclog::Write("Combat.SelfStatus", "no creature -> [%s]", phrase);
        return;
    }

    void* clientLeader = acc::engine::GetClientLeader();
    int hpCur = 0, hpMax = 0;
    acc::engine::ReadClientCreatureHitPoints(clientLeader, &hpCur, &hpMax);
    hpCur = (std::max)(hpCur, 0);
    hpMax = (std::max)(hpMax, 0);

    char msg[384];
    acc::strfmt::FixedText b{msg, sizeof(msg)};

    // Some creature shapes (driving and minigame slots) leave the max at 0:
    // then the current value alone, rather than "X of 0".
    if (hpMax > 0 && hpCur <= hpMax + 32) {
        b.Append(acc::strings::Get(S::FmtSelfStatusHpOf), hpCur, hpMax);
    } else {
        b.Append(acc::strings::Get(S::FmtSelfStatusHp), hpCur);
    }

    // Force pool, only for Force users: the max is 0 for the other classes
    // and for droids.
    int fpCur = 0, fpMax = 0;
    if (acc::engine::ReadCreatureForcePoints(clientLeader, &fpCur, &fpMax) &&
        fpMax > 0) {
        if (fpCur < 0) fpCur = 0;
        b.Append(acc::strings::Get(S::FmtSelfStatusFpOf), fpCur, fpMax);
    }

    Loadout loadout;
    AppendLoadout(b, creature, loadout);

    acc::speech::Speak(msg, /*interrupt=*/true);
    acclog::Write("Combat.SelfStatus",
                  "hp=%d/%d fp=%d/%d effects=[%s] main=[%s] off=[%s] -> [%s]",
                  hpCur, hpMax, fpCur, fpMax,
                  loadout.effects, loadout.mainWpn, loadout.offWpn, msg);
}

}  // namespace

acc::strings::Id DamageLevelStringId(int level) {
    using S = acc::strings::Id;
    static const S kIds[] = {
        S::DamageLevel0Healthy, S::DamageLevel1Light, S::DamageLevel2Wounded,
        S::DamageLevel3Badly,   S::DamageLevel4Dying, S::DamageLevel5Dead,
    };
    return kIds[level >= 0 && level <= 5 ? level : 0];
}

bool BuildTargetCombatBrief(void* targetServerObject, const char* targetName,
                            char* outBuf, size_t outBufSize) {
    outBuf[0] = '\0';
    // Creatures only: doors, items and waypoints get their suffixes in
    // engine_area::GetObjectName.
    if (acc::engine::GetObjectKind(targetServerObject) !=
        static_cast<int>(acc::engine::GameObjectKind::Creature)) {
        return false;
    }

    using S = acc::strings::Id;
    acc::strfmt::FixedText b{outBuf, outBufSize};
    b.Append(acc::strings::Get(S::FmtTargetCombatBrief), targetName);

    // Condition: the damage-level bucket a sighted player reads from the
    // colour of the HP bar. Healthy is not said.
    const int dl = acc::engine::ReadObjectDamageLevel(targetServerObject);
    if (dl > 0) {
        b.Append(acc::strings::Get(S::FmtBriefCondition),
                    acc::strings::Get(DamageLevelStringId(dl)));
    }

    // Distance in whole metres: the range a sighted player judges from the
    // target reticle.
    const float distM = PlayerDistanceMeters(targetServerObject);
    if (distM >= 0.0f) {
        b.Append(acc::strings::Get(S::FmtBriefDistanceMeters), WholeMetres(distM));
    }

    Loadout loadout;
    AppendLoadout(b, targetServerObject, loadout);

    acclog::Write("Combat.Brief",
                  "name=[%s] dl=%d distM=%.2f effects=[%s] main=[%s] off=[%s]",
                  targetName, dl, distM,
                  loadout.effects, loadout.mainWpn, loadout.offWpn);
    return true;
}

void PollWin32SelfStatusHotkey() {
    if (!acc::hotkeys::Pressed(acc::hotkeys::Action::SelfStatusAnnounce)) return;

    // Not before the player is loaded: reading HP, effects and inventory off
    // the half-initialised player slot between chargen and the world is a
    // known crash path.
    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) return;

    // Silent while a blocking panel is in front, so the readout never talks
    // over inventory, map or dialog speech.
    acc::engine::UiBlockState ui;
    if (acc::engine::IsForegroundUiBlocking(&ui)) {
        acclog::Write("Combat.SelfStatus",
                      "H suppressed — ui blocked (fg=%p kind=%s)",
                      ui.fgPanel,
                      ui.fgPanel ? acc::engine::PanelKindName(ui.fgKind) : "?");
        return;
    }

    SpeakSelfStatus();
}

}  // namespace acc::combat::query
