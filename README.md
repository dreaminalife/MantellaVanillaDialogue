# MantellaHeardLines

An SKSE plugin for [Mantella](https://github.com/art-from-the-machine/Mantella). It catches the voiced lines NPCs hear (scene lines, and the player's talk in the dialogue menu) so Mantella NPCs can remember them.

Skyrim reports each line as it starts. The plugin keeps scene lines and menu talk, and drops the player's own lines, Mantella's lines (`Mantella.esp`), lines from people outside the area the game has loaded around the player (it runs their scenes without speaking them, so nobody could hear them) and chatter: greetings, idle remarks, combat shouts, grunts and breathing. For each line it notes who spoke, to whom, the words (read from the line, so subtitles can be off), who heard it and whether the player was there. The lines wait in the save until the Mantella game add-on hands them to Mantella with a request it already sends. Nothing runs while nobody speaks.

One DLL works with Skyrim SE, AE and VR. It needs SKSE and the Address Library for SKSE Plugins.

## Papyrus

```papyrus
Scriptname MantellaHeardLines Hidden

Bool Function SetSettings(Bool catchLines, Bool nearbyOverhear, Float hearingDistance, Int mostWaiting) Global Native
String Function TakeLines() Global Native
```

- `SetSettings`: Vanilla Dialogues on or off, whether NPCs near a line also hear it, Hearing Distance in game units, and how many lines wait at most (the oldest are dropped). It returns false and changes nothing when the distance or the most waiting lines aren't above 0. Nothing is caught until settings arrive, so the add-on sends them on every load and on a new game.
- `TakeLines`: every waiting line as JSON, and the store is emptied. Always returns JSON, with the game's time now (left out while the game can't tell it), so Mantella can tell when an older save was loaded:

```json
{"now": {"day": 42.37, "hour": 14.5}, "player": "Kit",
 "lines": [{"day": 42.30, "hour": 12.9, "time": 1763600000, "place": "The Bannered Mare", "scene": 123456, "menu": false,
            "speaker": {"id": 1234567, "name": "Mikael"}, "to": {"id": 7654321, "name": "Carlotta"},
            "words": "Your beauty outshines the sun.", "asked": "",
            "heard_by": [{"id": 1234567, "name": "Mikael"}, {"id": 7654321, "name": "Carlotta"}, {"id": 1112223, "name": "Saadia"}],
            "player_there": true}]}
```

`heard_by` never holds the player. `to` is left out when nobody is spoken to. For menu talk, `menu` is true, `asked` is the player's pick and `to` is the player.

## Code

- `src/logic/`: plain logic with no game types: which lines are kept (`line_filter`), who heard a line (`hearing`), the waiting lines and their JSON (`line_store`), and the settings. The unit tests in `tests/` build these without CommonLib.
- `src/game_line_reader`, `src/game_actor_scan`, `src/co_save_record`: turn game objects and the SKSE co-save into that plain data.
- `src/line_catcher`: the event sink. The line is read where the game reports it, and who heard it is worked out on the main thread.
- `src/papyrus_api`: the two natives.

Caught lines are written to the log at debug level: `Documents\My Games\Skyrim Special Edition\SKSE\MantellaHeardLines.log`.

## Building

The DLL is built by GitHub Actions on every push (`.github/workflows/build.yml`). The unit tests run first, and a failing test fails the run. Download the DLL from the run's Artifacts. To build locally you need VS 2022 with the C++ workload and [xmake](https://xmake.io) 3.0+:

```bat
git clone --recurse-submodules <this repository>
xmake config --mode=releasedbg
xmake build MantellaHeardLinesTests
xmake run MantellaHeardLinesTests
xmake build MantellaHeardLines
```

The output is in `build/windows/x64/releasedbg/`.

## License

The source code in this repository is MIT licensed (see `LICENSE`). The built DLL statically links [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR) (`lib/commonlibsse-ng`), which is GPL-3.0 with a modding exception, so the DLL is distributed under those terms.

Started from the [commonlibsse-ng-template](https://github.com/libxse/commonlibsse-ng-template).
