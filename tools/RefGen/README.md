# RefGen

RefGen generates the test cooks in `tests/Paks`. The build, the tests and CI never run it. Regenerating the cooks
needs every launcher engine from UE 4.3 to 5.9 installed on one Windows machine, plus the matching Visual Studio
toolsets, which hardly anyone has. The scripts live here so that the cooks can be
reproduced and extended. Change them together with the cooks that they produce.

## What it produces

For each engine, RefGen creates one asset of every asset, actor and component type that the engine has. It then
cooks them and writes the reference values that the asset tests compare against:

- `tests/Paks/Windows/<x.y.z>.pak`: the 4.x cooks, each one encrypted, compressed pak with D3D shaders. Before 4.25
  the shader code is inline in the assets; later versions use a shared shader library.
- `tests/Paks/Windows/<x.y.z>.pak`, `.utoc`, `.ucas`, plus `<x.y.z>-global.utoc` and `.ucas`: the 5.x cooks, as IoStore containers.
- `tests/Paks/Windows-Vulkan/`: one cook with SPIR-V shaders.
- `<x.y.z>.json` next to each cook: `{"assets": {object path: class}, "values": {kind: {object path: value}}}`,
  computed by the engine itself (`lib/refgen_expected.py`).

The cooks are versioned (`-VersionCookedContent`), and the AES key is `RefGenTestKey0123456789abcdefghi`.

## Layout

| Path | Purpose |
| --- | --- |
| `RefGen/` | The project. The `RefGenEditor` module and its `RefGen` commandlet create the assets (`RefGenCatalog.cpp` lists them). |
| `RefGenReference/` | An editor plugin that evaluates the generated assets in the engine, for the reference values. |
| `SourceAssets/` | The imported source files: FBX meshes and animation, WAV, IES and FGA. `make_fbx.py` writes the FBX files. |
| `build_refgen.ps1` | Stages the project for one engine, ports its source to that engine and builds `RefGenEditor`. |
| `gen_refgen.ps1` | Runs the commandlet, which generates the assets. |
| `trio.ps1` | Builds and generates for a comma-separated list of engines. |
| `lib/sweep_win.py` | The whole pipeline for each engine: build, generate, bake (`lib/bake_win.py`: lighting, HLOD, reference values), cook (`lib/cook_win.py`). `sweep.cmd` runs it for every engine. |
| `lib/fetch_windows_paks.sh` | Copies the cooks and reference values from the machine (ssh host `windows`) into `tests/Paks`. |
| `lib/fix_uproject.py`, `lib/ue4_compat.py`, `lib/resolve_has_include.py` | Port the project to older engines. |
| `lib/egs_fetch.py`, `lib/egs_restore.py`, `restore_all.ps1` | Install engines that the launcher no longer offers, and restore the engine packages that an editor resaved. |
| `sdkpatch.ps1` | Patches the Windows SDK so that the VS2015 (v140) toolset of UE 4.19 and older can compile against it. |

## Machine setup

The scripts expect this setup:

- This folder at `C:\RefGenWin\tool`, and the `.ps1` and `.cmd` files in `C:\RefGenWin`.
- Python at `C:\Python312`.
- The engines in `C:\Program Files\Epic Games\UE_<x.y>`.
- These toolsets: VS2015 (v140) for 4.19 and older, the VS2017 build tools (MSVC 14.16) for 4.20 and 4.21, and
  VS2022 for newer engines. 5.2 and 5.3 need MSVC 14.38. `build_refgen.ps1` picks the toolset through
  `REFGEN_MSVC`, which `RefGenEditor.Target.cs` applies.

To regenerate, run `sweep.cmd` on the machine, then run `lib/fetch_windows_paks.sh <CppUE5Parse checkout>`. To
regenerate one engine, use `python lib\sweep_win.py UE_5.1`.
