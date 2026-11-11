# Security Policy

## Scope

This is a personal fork (aradix85, branch `bridge`) of Voice of the Old
Republic, a local accessibility mod for Star Wars: Knights of the Old Republic
1 and 2. The mod:

- Injects a DLL into the game process to read the game's in-memory UI and world
  state and pass it to NVDA as speech and braille.
- Sends no telemetry, no user data, and no logs anywhere. Logs are written only
  to local files on your own machine.
- Makes no outbound network call. The original mod's update check is gone.
- Runs a local HTTP bridge for helper tools. It listens on 127.0.0.1 only. Its
  port and a token that is new for every game session are written to
  `bridge.json` next to the game executable, and every request must carry the
  token in the `X-Kotor-Bridge` header. With the token a local program can read
  what the mod knows (area, party, objects, the spoken lines) and make the game
  act (set the target, walk, use an object, press a key, speak a line).

The installer is the part that downloads: the mod from this fork's
`bridge-latest` pre-release on GitHub, and the community patches it offers from
GitHub and DeadlyStream, pinned in `Resources/sources.json` (see
`docs/installer.md`). It asks for administrator rights.

There is no server component, no account system, and no credentials handled by
the mod.

## Reporting a vulnerability

Report it privately through GitHub: on this repository, Security, then
"Report a vulnerability"
(https://github.com/aradix85/voice-of-the-old-republic/security/advisories/new).
Issues are switched off on this fork. A problem that is also in the original
mod belongs with its maker:
https://github.com/JeanStiletto/voice-of-the-old-republic.

## Realistic attack surface

- **The bridge.** Any program that runs as the same Windows user can read
  `bridge.json` and drive the game; that crosses no boundary. A real issue
  would be a way to reach the bridge from another machine or from a web page,
  or to make it answer or act without the token.
- **The installer's downloads.** It fetches over HTTPS and installs with
  administrator rights. A real issue would be a way to make it install a file
  that is not the published one: a spoofed release response, a hijacked
  download URL, a writable path an attacker could pre-seed.

## Things that are explicitly out of scope

- **Data exfiltration** — the mod contains no code that sends your data
  anywhere.
- **Anti-cheat / account bypass** — the mod only reads and narrates state the
  game has already computed for a single-player, offline RPG. There is no
  multiplayer or account system to attack.
- **Windows SmartScreen / "unknown publisher" warnings** on the unsigned
  installer and DLL are a code-signing cost issue, not a security
  vulnerability.
