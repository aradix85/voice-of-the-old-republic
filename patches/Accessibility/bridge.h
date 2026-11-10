// Local bridge: lets an external assistant (Claude, through kotor-tools'
// kotor.py) read live game state and drive a few actions while the player
// plays. Loopback only (127.0.0.1), HTTP/1.0, JSON answers.
//
// Every request must carry the header "X-Kotor-Bridge: <token>". The token
// is random per session and written, with the port, to bridge.json next to
// swkotor.exe. A browser page cannot set that header on a cross-origin
// request without a CORS preflight this server never answers, so web pages
// cannot drive the game through it.
//
// Threading: a background thread owns the socket. Engine state is only read
// on the main thread, inside Tick() (called from core_tick's Dispatch).
// /health and /spoken are answered on the socket thread; every other path
// queues a job that the next Tick() runs, and the socket thread waits up to
// 2 s for it (the game does not tick while it is minimised, so a timeout
// answers with an error instead of hanging).
//
// Endpoints:
//   GET /health
//   GET /spoken?since=<id>&max=<n>      lines the mod spoke, oldest first
//   GET /state                          area, position, party, screen, target
//   GET /menu                           the open menu as the mod walks it
//   GET /nearby?radius=<metres>         what the mod's cycle lists, nearest first
//   GET /overview                       regions explored, nearest unexplored, loot left
//   GET /objects                        every door, container, item and creature
//                                       with tag and live state, for kotor_kaart.py
//   GET /rooms                          the mod's room graph of the area
//   GET /do?action=select|interact|use&handle=<hex or dec>
//   GET /do?action=walk&x=<f>&y=<f>
//   GET /do?action=say&text=<utf-8, url-encoded>
//   GET /do?action=key&scancode=<dec or 0x..>[&extended=1]
// All but /health, /spoken, /state, /menu and the say and key actions are
// KOTOR 1 only for now.

#pragma once

namespace acc::bridge {

// Main thread, once per frame. Starts the server on first call.
void Tick();

// Record one utterance (game codepage) into the ring buffer. Called from the
// speech capture path; safe from any thread.
void RecordSpoken(const char* text);

}  // namespace acc::bridge
