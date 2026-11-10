// Cycle input: the two ingestion paths (the engine key hook and the Win32
// poll) and the category table both the announce and the focus handlers
// use.

#include "cycle_input_internal.h"

#include "engine_input.h"
#include "engine_panels.h"
#include "engine_player.h"
#include "hotkeys.h"
#include "peek_description.h"
#include "strings.h"

namespace acc::cycle_input {

namespace {

// Engine-side shift flag, mutated by manager-level shift up/down events.
// The manager doesn't bake shift into a modifier flag — DirectInput delivers
// each key as its own event — so we track it ourselves. The Win32 polling
// path queries GetAsyncKeyState directly (no need to share this flag).
bool g_engineShiftHeld = false;

}  // namespace

CategoryBindings BindingsFor(
        acc::filter::CycleCategory c,
        acc::filter::CycleContext ctx) {
    using S = acc::strings::Id;
    using C = acc::filter::CycleCategory;
    switch (c) {
        case C::Door:
            return {S::CategoryDoor,       S::EmptyDoors};
        case C::Npc:
            return {S::CategoryNpc,        S::EmptyNpcs};
        case C::Container:
            return {S::CategoryContainer,  S::EmptyContainers};
        case C::Item:
            return {S::CategoryItem,       S::EmptyItems};
        case C::Landmark:
            // Map context renames to "Map hint" — matches the
            // engine's own GetNext/PrevMapNote terminology so the spoken
            // category word lines up with what sighted players see in the
            // InGameMap up/down buttons. World context keeps the broader
            // "Landmark" wording for the unrestricted has_map_note set.
            if (ctx == acc::filter::CycleContext::Map) {
                return {S::CategoryMapHint, S::EmptyMapHints};
            }
            return {S::CategoryLandmark,   S::EmptyLandmarks};
        case C::Transition:
            return {S::CategoryTransition, S::EmptyTransitions};
        case C::Count_:
            break;
    }
    return {S::CategoryItem, S::EmptyAll};  // unreachable safety net
}

bool TryHandleEvent(int param_1, int param_2) {
    // Shift state tracking — fires on both press and release, never consumed.
    // Notify peek_description on the release edge so its block cursor resets
    // (so the next Shift+arrow starts at block 0 again).
    if (param_1 == kInputKbLeftShift || param_1 == kInputKbRightShift) {
        bool wasHeld = g_engineShiftHeld;
        g_engineShiftHeld = (param_2 != 0);
        if (wasHeld && !g_engineShiftHeld) {
            acc::peek::OnShiftReleased();
        }
        return false;
    }

    // Press only beyond this point.
    if (param_2 == 0) return false;

    // Filter to the three codes we care about before any further work.
    if (param_1 != kInputKbComma  &&
        param_1 != kInputKbPeriod &&
        param_1 != kInputKbAnnounce) {
        return false;
    }

    // Gate on "is the player actually in-game?". GetPlayerPosition returns
    // false on menus / chargen pre-spawn / area-load mid-flight, in which
    // case we let the key pass through to its normal engine handler.
    Vector playerPos;
    if (!acc::engine::GetPlayerPosition(playerPos)) return false;

    // Map sub-screen captures the cycle keys when it's foreground (under
    // the InGameMenu strip — same scan pattern map_ui_cursor uses). World
    // and Map cycle states are independent; the user can return to
    // in-world cycle exactly where they left off after closing the map.
    acc::filter::CycleContext ctx =
        acc::engine::HasActiveMapPanel()
            ? acc::filter::CycleContext::Map
            : acc::filter::CycleContext::World;

    if (param_1 == kInputKbComma || param_1 == kInputKbPeriod) {
        // In-world `,`/`.` are always live: by default they drive the
        // DISCOVERY tier (only objects the player has organically had
        // narrated), and the "Extended cycling" mod setting widens the same
        // keys to everything in the area. That discovery-vs-extended filter
        // lives in cycle_state::BuildCategoryListing — here we always
        // dispatch. The `-` family (announce / autowalk / route) maps to
        // kInputKbAnnounce below and is unaffected.
        const bool prev = (param_1 == kInputKbComma);
        // Ctrl+, / Ctrl+. jump to the first / last item of the current
        // category. Ctrl wins over Shift (matches the dash-family
        // Ctrl>Alt>Shift precedence). The engine latches Shift for us
        // (g_engineShiftHeld) but not Ctrl, so read OS-level Ctrl state
        // directly — same source PollWin32's binding match uses.
        const bool ctrl = acc::hotkeys::CtrlHeld();
        if (ctrl)              OnCycleEdge    (/*last=*/!prev, ctx);
        else if (g_engineShiftHeld) OnCycleCategory(prev, ctx);
        else                   OnCycleItem    (prev, ctx);
        // Suppress PollWin32 from re-dispatching the same press. In Map
        // context the engine routes `,`/`.` through the manager hook
        // AND PollWin32 still sees the OS-level press → without claim
        // the user hears each press dispatched twice. ClaimRisingEdge
        // (not Consume) is required because the manager input fires
        // between EndTick and BeginTick — see the InteractTarget claim
        // in menus.cpp::OnHandleInputEvent for the same pattern.
        if (ctrl) {
            acc::hotkeys::ClaimRisingEdge(
                prev ? acc::hotkeys::Action::CycleItemFirst
                     : acc::hotkeys::Action::CycleItemLast);
        } else if (g_engineShiftHeld) {
            acc::hotkeys::ClaimRisingEdge(
                prev ? acc::hotkeys::Action::CycleCategoryPrev
                     : acc::hotkeys::Action::CycleCategoryNext);
        } else {
            acc::hotkeys::ClaimRisingEdge(
                prev ? acc::hotkeys::Action::CycleItemPrev
                     : acc::hotkeys::Action::CycleItemNext);
        }
        return true;
    }
    if (param_1 == kInputKbAnnounce) {
        if (g_engineShiftHeld) OnPathfindFocus();
        else                   OnAnnounceFocus();
        return true;
    }
    return false;
}

void PollWin32() {
    // All bindings live in `hotkeys.cpp`. Modifier-precedence at the
    // announce / autowalk / force / route key is encoded via per-Action
    // `modsRequired` + `modsForbidden`, so each Action::* below is
    // mutually exclusive given the current keyboard state.
    namespace hk = acc::hotkeys;

    bool risingCommaItem     = hk::Pressed(hk::Action::CycleItemPrev);
    bool risingCommaCategory = hk::Pressed(hk::Action::CycleCategoryPrev);
    bool risingPeriodItem    = hk::Pressed(hk::Action::CycleItemNext);
    bool risingPeriodCategory= hk::Pressed(hk::Action::CycleCategoryNext);
    bool risingCommaFirst    = hk::Pressed(hk::Action::CycleItemFirst);
    bool risingPeriodLast    = hk::Pressed(hk::Action::CycleItemLast);
    bool risingAnnounce      = hk::Pressed(hk::Action::AnnounceFocus);
    bool risingPathfind      = hk::Pressed(hk::Action::PathfindFocus);
    bool risingPathfindForce = hk::Pressed(hk::Action::PathfindFocusForce);
    bool risingRoute         = hk::Pressed(hk::Action::RouteFocus);

    if (!risingCommaItem && !risingCommaCategory &&
        !risingPeriodItem && !risingPeriodCategory &&
        !risingCommaFirst && !risingPeriodLast &&
        !risingAnnounce && !risingPathfind &&
        !risingPathfindForce && !risingRoute) return;

    // Gate on in-world. In menus / chargen / pre-spawn this short-circuits
    // before any cycle effect. (Foreground gate already applied by Pressed().)
    Vector playerPos;
    if (!acc::engine::GetPlayerPosition(playerPos)) return;

    // Same context split as TryHandleEvent — map foreground routes cycle
    // keys to the map-cycle state singleton, otherwise world.
    acc::filter::CycleContext ctx =
        acc::engine::HasActiveMapPanel()
            ? acc::filter::CycleContext::Map
            : acc::filter::CycleContext::World;

    // In-world `,`/`.` (item + category, both shift variants) are always
    // live now: by default they drive the DISCOVERY tier; the "Extended
    // cycling" mod setting widens the listing to everything. The
    // discovery-vs-extended filter lives in cycle_state::BuildCategoryListing,
    // so input always dispatches here. Map context was always live and stays
    // so. The `-` family below stays unaffected.
    {
        if (risingCommaItem)      OnCycleItem    (/*prev=*/true,  ctx);
        if (risingCommaCategory)  OnCycleCategory(/*prev=*/true,  ctx);
        if (risingPeriodItem)     OnCycleItem    (/*prev=*/false, ctx);
        if (risingPeriodCategory) OnCycleCategory(/*prev=*/false, ctx);
        if (risingCommaFirst)     OnCycleEdge    (/*last=*/false, ctx);
        if (risingPeriodLast)     OnCycleEdge    (/*last=*/true,  ctx);
    }

    // Precedence: Ctrl > Alt > Shift > bare. The Action bindings encode
    // this via mutually-exclusive modifier masks, so at most one of these
    // four can be true on a given tick. Route wins ties (Ctrl+Shift+-
    // speaks the route, not autowalk), then ForceWalkTo (Alt — diagnostic
    // queue-bypass kept for future companion-NPC nudges), then autowalk
    // via UseObject (Shift), then bare repeat-announce.
    if (risingRoute)              OnRouteFocus();
    else if (risingPathfindForce) OnPathfindFocusForce();
    else if (risingPathfind)      OnPathfindFocus();
    else if (risingAnnounce)      OnAnnounceFocus();
}

}  // namespace acc::cycle_input
