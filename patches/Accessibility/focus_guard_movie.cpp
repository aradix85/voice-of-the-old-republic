// The movie-end foreground guard. A cutscene's movie handoff can leave no
// window at all owning the foreground (the K2 Peragus docking stunt); the
// engine auto-pauses on the focus loss and never wakes, which to a blind
// player is "Paused" and then silence. For a short time after each movie end
// the guard reclaims the render window, but only while the foreground is
// orphaned: a foreign owner is a deliberate alt-tab, our own owner means
// nothing is wrong, and a new movie segment must keep its focus (forcing it
// away mid-playback can abort the movie queue and close the game).

#include "focus_guard_internal.h"

#include <windows.h>

#include "bringup_announce.h"
#include "log.h"

namespace acc::focus_guard {

namespace {

// Each movie end (every segment of a multi-segment cutscene) re-arms the
// window; the attempt cap keeps a persistently orphaned foreground from
// turning into a focus war.
constexpr DWORD kMovieReclaimWindowMs = 2000;
constexpr int   kMovieReclaimMax      = 3;

// Poll thread only.
DWORD g_deadline = 0;   // GetTickCount() deadline; 0 = disarmed
int   g_reclaims = 0;
bool  g_moviePlayingPrev = false;

void Disarm(const char* why) {
    g_deadline = 0;
    acclog::Write("Focus", "MovieEndForegroundGuard: %s; disarmed (reclaims=%d)", why, g_reclaims);
}

}  // namespace

void TickMovieEndGuard(HWND liveRender) {
    const bool playing = acc::bringup_announce::IsMoviePlaying();
    if (g_moviePlayingPrev && !playing) {
        g_reclaims = 0;
        g_deadline = GetTickCount() + kMovieReclaimWindowMs;
        if (g_deadline == 0) g_deadline = 1;
        acclog::Write("Focus", "MovieEndForegroundGuard: armed for %lu ms after movie end",
                      kMovieReclaimWindowMs);
    }
    g_moviePlayingPrev = playing;

    if (g_deadline == 0 || playing) return;
    if (static_cast<int>(GetTickCount() - g_deadline) >= 0) {
        Disarm("window elapsed");
        return;
    }
    if (!liveRender || !IsWindow(liveRender) || GetForegroundWindow() != nullptr) return;
    if (g_reclaims >= kMovieReclaimMax) {
        Disarm("reclaim cap reached");
        return;
    }
    ++g_reclaims;
    acclog::Write("Focus", "MovieEndForegroundGuard: foreground orphaned (NULL) after movie; "
                  "reclaiming render %p (attempt %d/%d)", liveRender, g_reclaims,
                  kMovieReclaimMax);
    // With no foreground owner there is no foreground lock to work around.
    BringWindowToTop(liveRender);
    SetForegroundWindow(liveRender);
}

}  // namespace acc::focus_guard
