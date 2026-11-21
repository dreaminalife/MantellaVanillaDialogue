# MantellaHeardLines

An SKSE plugin for [Mantella](https://github.com/art-from-the-machine/Mantella). It catches the voiced lines NPCs hear (scene lines, and the player's talk in the dialogue menu) so Mantella NPCs can remember them.

**This is the test build.** It only writes every line Skyrim reports to its log, so we can check what the game gives us. It keeps nothing and hands nothing to Mantella.

One DLL works with Skyrim SE, AE and VR. It needs SKSE and the Address Library for SKSE Plugins.

## What to check in game

The log is `Documents\My Games\Skyrim Special Edition\SKSE\MantellaHeardLines.log`. Each spoken line writes a `BEGIN` block and an `END` line.

1. In Whiterun, watch a scene, such as Mikael and Carlotta in the Bannered Mare.
2. Talk to an NPC through the dialogue menu, and pick one of Mantella's own menu lines.
3. Talk to an NPC through Mantella.
4. Turn subtitles off and watch a scene again.

Things to look for:

- Scene lines show up, with `subtype=14` or a `speaker_scene` other than 0.
- The words are there with subtitles off, in `words`.
- Mantella's own lines show `file=Mantella.esp`.
- In a two-person scene, `target` or the scene's aliases show who is spoken to.
- In the dialogue menu, `pick` shows what the player chose.
- `stage`: whether one line gives one `BEGIN` or one per response.
- `main_thread`: whether the game reports lines on its main thread.
- `day`, `hour`, `global_present` and the place.

Also note the scene's form ID and its quest (`scene_quest`), and the console commands that start the scene, for the later guided test.

## Building

The DLL is built by GitHub Actions on every push (`.github/workflows/build.yml`); download it from the run's Artifacts. To build locally you need VS 2022 with the C++ workload and [xmake](https://xmake.io) 3.0+:

```bat
git clone --recurse-submodules <this repository>
xmake config --mode=releasedbg
xmake build
```

The output is in `build/windows/x64/releasedbg/`.

## License

The source code in this repository is MIT licensed (see `LICENSE`). The built DLL statically links [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR) (`lib/commonlibsse-ng`), which is GPL-3.0 with a modding exception, so the DLL is distributed under those terms.

Started from the [commonlibsse-ng-template](https://github.com/libxse/commonlibsse-ng-template).
