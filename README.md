# Voice of the Old Republic, Anouk's version

This is a fork of [Voice of the Old Republic](https://github.com/JeanStiletto/voice-of-the-old-republic) by JeanStiletto, the mod that makes Star Wars: Knights of the Old Republic playable for fully blind players. It keeps what the original does — speech for menus, dialogue and combat, navigation helpers and accessible minigames — and changes it in a few deliberate ways for one player, Anouk, who plays with NVDA and a braille display.

## How this version differs from the original

- **NVDA only, with braille.** Every line goes to NVDA as speech and as a braille message, through NV Access's controller client. There is no support for other screen readers, no Windows (SAPI) voices and no positional 3D speech. When NVDA is not running, the mod is silent.
- **Words instead of sounds.** The exploration sounds (walls, doors, objects, collisions, the guidance beacon) are gone; what they told you is now said in words. The only sounds the mod adds are the cues of the swoop race and the turret minigame.
- **English only.** The mod and its installer speak English, and the mod assumes the English version of the game.
- **Keyboard only.** The original's gamepad support is gone; the key list (F1) and the tutorial hints name keyboard keys. KOTOR 2's own controller support, which is part of the game, is untouched.
- **More help finding your way.** Rooms, doors and routes are described in more detail, and the mod remembers what you have seen. See [Finding your way](#finding-your-way).
- **A local bridge for helper tools.** Tools on the same PC can read the game state and act in the game. See [The bridge](#the-bridge).
- **Builds come from this fork.** Every change is built automatically, and the installer downloads the mod from here.

## Requirements

- Windows 10 or later
- Star Wars: Knights of the Old Republic, version 1.0.3, Steam or GOG, in English
- NVDA
- About 200 MB of free disk space

Knights of the Old Republic II: the installer can set the mod up for it and the game speaks, but it has not been played through, and its minigames and tutorial messages are not covered yet. Only Aspyr's current build (Steam and GOG) is supported.

## Installing, updating, uninstalling

Everything is in the release [bridge-latest](https://github.com/aradix85/voice-of-the-old-republic/releases/tag/bridge-latest), which GitHub rebuilds after every change.

1. Close the game. If you have a game in progress, back up your saves first (`Documents\Swkotor\saves`).
2. Download `VoiceOfTheOldRepublicInstaller.exe` from bridge-latest.
3. Right-click it and choose Run as administrator. Windows SmartScreen warns about an unknown publisher, because the installer is not code-signed: choose More info, then Run anyway.
4. Step through the screens. The installer finds your game, installs the patch framework, the mod, NVDA's controller client and the widescreen fix, and offers the K1 Community Patch.
5. Start the game from the last screen or from Steam.

**Updating:** run the installer again; it tells you when a newer build is out. Or close the game and copy `accessibility.dll` from bridge-latest into the game's `patches` folder.

**Uninstalling:** run the installer again and choose uninstall, or use Installed apps in Windows Settings. Only this mod's files are removed; the K1 Community Patch and other optional mods stay. For a completely clean game, let Steam verify the game files or reinstall it from GOG afterwards.

## First steps

A new game starts with **character creation**: class, portrait, attributes, skills and feats. The mod reads each panel as you move through it; nothing is timed.

You then wake on the **Endar Spire**, a Republic ship under attack: the game's tutorial. The mod replaces the game's tutorial popups with keyboard hints written for screen-reader users, so you learn the controls as you go. Follow Trask toward the escape pods. After the Endar Spire you reach **Taris**, the first large world.

A few habits that help from the start:

- **H** gives your health and status, **Ctrl+H** the health of the whole party, and **F1** the full list of keys.
- **Q / E** find things around you; **`,` / `.`** get you back to things you have found; **Ctrl+N** tells you where you are and what is left.
- **Press O** for the game's Options early. Under Gameplay you set the auto-pause, which decides when combat stops on its own, and the key mapping. At the bottom of the list is **Mod settings**.

## Finding your way

- **Q / E** step through what is around you — creatures, doors and usable objects — each with its name, clock direction and distance. With "Q and E: everything around you" on in Mod settings, they step through everything within 30 metres that is not behind a wall, not only what the camera sees, and in combat through the living enemies.
- **`,` / `.`** step through what you have already found in this area, grouped by category: people, doors, containers, items, area exits, landmarks and your own map markers. Only what you can still reach is listed: nothing behind a locked door, no empty containers, no decorative doors. Party members are always in the people list, and the game's own map notes are always in the landmark list, so Enter can walk you there before you have been near them.
- **Entering a room** speaks its name, its shape (corridor, junction, dead end, open space) and its exits, computed live from the game's walkmesh. Each exit says where it leads and whether you have been there, then "Here:" lists everything in the room: people, containers with something in them, items on the floor, found or not.
- **Doors tell you what is behind them.** A door to a room you visited names what is left there ("visited, Matrik", or "nothing left"); a door to another area you have been through names that area's map notes. Locked doors and containers say how they open: "needs a key", "Security 6, best in your party: Carth with 4", "opens only through the story", or "may open when you try" for a door whose story script runs when you try it.
- **Routes.** For anything in another room, the focus line gives the way there ("via Door 5 East, about 45 metres to walk") or "behind a locked door". Ctrl+`/` (Ctrl+`-` on a German keyboard) describes the route step by step.
- **The overview, Ctrl+N:** the room you are in, how much of the area you have explored, the nearest unexplored place and the way there, what is left to search, and the way out.
- **Walking for you.** Enter walks to the focused object and uses it. Ctrl+Shift+N walks all the way to the nearest unexplored place, opening doors on the way, and tries a locked door when nothing else is left. Ctrl+L walks to the nearest container or item left to search and opens it. When something blocks the way, the mod steps aside and tries again, up to three times, and otherwise says who is in the way. Enter on an open door walks through it; on a closed door to another area, it walks through as soon as the door opens.
- **Reminders.** Enter on a door while something in the room is still left to search gives "Still here: …". When a door nearby opens that you did not open, you hear "Door 3 opens".
- **Orientation.** AltGr says which way you face and the area you are in. N turns the camera to the nearest exit, and each further press to the next one clockwise.

The mod keeps in your save which rooms you visited and where the doors to other areas lead, so this survives saving and loading.

## Keyboard

The mod keeps the game's key map, with one change the installer makes on a fresh install: strafe on A / D and camera rotation on Z / C (the game's own default is the other way round). The game's keys can be re-bound in Options, Gameplay, Key Mapping; the keys the mod adds in Mod settings, Key bindings.

### Game keys you will use most

- W / S — Move forward / backward
- A / D — Strafe left / right
- Z / C — Rotate the camera left / right
- Q / E — Previous / next target
- R — Default action on the target (attack, talk, open)
- 1 / 2 / 3 — The three actions on the target's action menu
- 4 / 5 / 6 / 7 — Your Force power / medpac / item / mine slots
- Tab — Change party leader
- F — Cancel combat, G — Stealth, V — Solo mode, X — Flourish weapon
- Space — Pause
- Esc — Game menu
- F4 — Quick save, F5 — Quick load
- I — Inventory, U — Equipment, P — Character record, K — Skills, feats and powers
- M — Map, L — Quests, J — Messages, O — Options

### Mod keys — the world

- Enter — Default action on the focused target, or walk to it
- Shift+Enter — The action menu for the target: every action (attack, talk, Force powers, items, special abilities) in one menu
- Shift+1 … Shift+7 — One action category (1–3 the target's actions, 4–7 your Force powers, items and mines)
- H — Your health, active effects and weapon
- Ctrl+H — Health of everyone in the party, and who can level up
- Shift+H — The action queue (review or clear queued actions)
- Shift+L — Level up
- F1 — The full key list; Ctrl+F1 — the keys for the current screen
- Ctrl+R — Copy the last spoken text to the clipboard

### Mod keys — found objects

- `,` / `.` — Previous / next object in the current category
- Shift+`,` / Shift+`.` — Previous / next category
- Ctrl+`,` / Ctrl+`.` — Nearest / farthest object in the category
- `/` (`-` on a German keyboard) — Say the focused object again
- Shift+`/` — Walk to it
- Ctrl+`/` — Describe the route to it, step by step

### Mod keys — orientation

- AltGr (right Alt, alone) — Facing and area
- N — Camera to the nearest exit; again for the next one clockwise. Where no exits are known, it turns 90 degrees clockwise
- Ctrl+N — Exploration overview
- Ctrl+Shift+N — Walk to the nearest unexplored place
- Ctrl+L — Walk to the nearest container or item left to search, and open it

### View mode

B turns view mode on and off. In view mode the character stands still:

- A / D — Turn the camera
- W / S — Move the view cursor; it stops at walls and says "Wall"
- Enter — Use what the camera points at, or walk to that point
- Shift+Enter — The action menu for what the camera points at

### Map screen

- Up / Down — The map's notes and landmarks
- `,` / `.` — Your map markers and the map's pins
- Shift+N — Drop a map marker at the cursor, named after the nearest room or landmark; Ctrl+`/` then describes the route to it
- Pushing the cursor against the edge of the map says "Edge of the map"

### Mod menus

In the action menu, a category menu or the action queue:

- Up / Down — Move
- Left / Right — Columns or variants
- Enter — Activate
- Shift+Enter — (action queue) Clear all queued actions
- Esc — Close

### Other screens

- Container — Q / E switch between taking and giving; Enter takes everything
- Store — Q / E switch between buying and selling
- Text fields (character name and others) — Up / Down read the text again, Enter submits, Esc cancels

### Pazaak

- Up / Down — Your hand, your table, the opponent's table, the actions
- Left / Right — Within the current row
- Enter — Play the focused card, or activate the focused action
- S — Stand, E — End turn
- C — Your hand, T — Both tables with their totals
- Shift+C — How many cards the opponent still holds
- On a plus/minus card, Enter opens a sign choice: Left / Right pick, Enter plays, Esc cancels

On the wager screen the top entry reads your bet, the table maximum and your credits. The side-deck builder reads every card and slot.

## Mod settings

Options (O), then Mod settings at the bottom of the list. Up / Down move, Enter toggles or opens, Left / Right move a slider, Esc goes back.

- Map-wide object selection — `,` / `.` also list objects you have not found yet
- Room shape descriptions — the room's name, shape and exits when you enter
- Read voiced-speaker subtitles — also speak the subtitles of voiced lines
- Q and E: everything around you — see [Finding your way](#finding-your-way)
- Autoaiming — aim assist in the turret minigame
- Skip launch intro movies — from the next start
- Hint sound volume — the volume of the minigame cues
- Key bindings — re-bind the keys the mod adds
- Audio glossary — plays each swoop-race cue with its name, so you know them before the race
- Support the modder — the original author's Ko-fi page
- Latest changes — this fork's changelog
- Open the guide — this page

## The bridge

While the game runs, the mod listens on 127.0.0.1 — this PC only — for tools that want to read the game state or act in the game. At every start it writes the port and a fresh token to `bridge.json` next to `swkotor.exe`; a request without that token is refused. It can report what is on screen, the party, what is nearby, the explored map and what NVDA said, and it can focus, use and walk to objects. Most of it works in KOTOR 1 only. The endpoints are listed in [patches/Accessibility/bridge.h](patches/Accessibility/bridge.h).

## Troubleshooting

### No speech

- Make sure NVDA is running.
- `nvdaControllerClient32.dll` must be in the game's `patches` folder, next to `accessibility.dll`. The installer puts it there.
- The newest log is `<game folder>\logs\patch-*.log`; the installer's Collect logs button gathers it for you.

### Lines are cut off while you hold a key

NVDA's "Speech interrupt for typed characters" (NVDA settings, Keyboard) stops the current line at every key press, including keys you hold, such as the map cursor or the turret's fire key. Turn it off if that bothers you.

### The game crashes at start, or the mod does not load

- Run the installer as administrator; it places the `dinput8.dll` loader that starts the mod with the game.
- The installer checks your game version and says so when it is not supported.
- A Steam or GOG update can overwrite the loader: run the installer again.

### Keys do nothing

- Make sure the game window has the focus.
- Press F1: if you hear the key list, the mod is running.
- Some keys work only in one place (the Pazaak keys on the Pazaak board, the menu keys inside a mod menu).

### The camera no longer turns

- You may have switched on the game's free look; Caps Lock toggles it by default. Press it again.
- Or view mode is on (B), where A / D turn the camera instead of moving you. Press B again.

## Known issues

See [docs/known-issues.md](docs/known-issues.md).

## For developers

- [CONTRIBUTING.md](CONTRIBUTING.md) — building, testing and the rules of the code
- [ARCHITECTURE.md](ARCHITECTURE.md) — how the mod is put together
- [ACCESSIBILITY_MODDING_GUIDE.md](ACCESSIBILITY_MODDING_GUIDE.md) — accessibility modding in general, and native-binary specifics
- [docs/CHANGELOG.md](docs/CHANGELOG.md) — what changed

## Credits

**Voice of the Old Republic** was created by **JeanStiletto**, a fully blind developer, with heavy help from Anthropic's Claude. Everything this fork builds on is their work, together with the people they thank:

- **Lane Dibello** — [Kotor-Patch-Manager](https://github.com/LaneDibello/Kotor-Patch-Manager), the reverse-engineered Ghidra database and the patch framework the mod runs on
- **NV Access** — NVDA and its controller client
- **The K1 Community Patch team** — the bundled bug-fix layer
- **xoreos and xoreos-tools** — the open-source engine reimplementation, a reference for the file formats
- **The DeadlyStream community** — the modding knowledge base
- The testers of the original mod: Kenny, Berenion, unexplained entity, Grinvold, dansc93, Mojsior, stirlock, Druidah, SightlessKombat, Destranis, Ozuaw, mkdbzfan, Kamilana, ABlindFellow, zargontheevilgod, zersiax and Michael

The changes in this fork are written by Claude (Anthropic) at Anouk's direction.

To support the original author: [Ko-fi: ko-fi.com/jeanstiletto](https://ko-fi.com/jeanstiletto).

## License

The mod's source is licensed under the GNU General Public License v3 (see [LICENSE](LICENSE)). Code under `third_party/` keeps its own license: the NVDA controller client is LGPL-2.1, Kotor-Patch-Manager follows its upstream terms, and dsoal and OpenAL Soft (only used when the optional spatial-audio setting is on) are LGPL-2.1. The game and BioWare's data files are not part of this project.

## Links

- [This fork](https://github.com/aradix85/voice-of-the-old-republic/tree/bridge)
- [The original Voice of the Old Republic](https://github.com/JeanStiletto/voice-of-the-old-republic)
- [Kotor-Patch-Manager](https://github.com/LaneDibello/Kotor-Patch-Manager)
- [DeadlyStream (K1 Community Patch)](https://deadlystream.com/)
