// Local bridge — lets an external assistant (Claude, via a small Python
// client) read live game state and drive a few actions while the player
// plays. Loopback only (127.0.0.1), HTTP/1.0, JSON answers.
//
// Every request must carry the header "X-Kotor-Bridge: <token>". The token
// is random per session and written, with the port, to bridge.json next to
// swkotor.exe. A browser page cannot set that header on a cross-origin
// request without a CORS preflight this server never answers, so web pages
// cannot drive the game through it.
//
// Threading: a background thread owns the socket. Engine state is only read
// on the main thread, inside Tick() (called from core_tick's Dispatch):
//   * /state is answered from a snapshot Tick() refreshes every 250 ms;
//   * /nearby and /do queue a job that the next Tick() runs, and the socket
//     thread waits up to 2 s for it (the game does not tick while it is
//     minimised, so a timeout answers with an error instead of hanging);
//   * /spoken reads a ring buffer that the speech path fills.
//
// Endpoints:
//   GET /health
//   GET /spoken?since=<id>&max=<n>
//   GET /state
//   GET /nearby?radius=<metres>
//   GET /do?action=select|interact|use&handle=<hex or dec>
//   GET /do?action=walk&x=<f>&y=<f>
//   GET /do?action=say&text=<utf-8, url-encoded>
//   GET /do?action=key&scancode=<dec or 0x..>

#pragma once

namespace acc::bridge {

// Main thread, once per frame. Starts the server on first call.
void Tick();

// Record one utterance (game codepage) into the ring buffer. Called from the
// speech capture path; safe from any thread.
void RecordSpoken(const char* text);

}  // namespace acc::bridge
