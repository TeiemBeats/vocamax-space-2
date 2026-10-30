cmake_minimum_required(VERSION 3.22)

project(VocaMaxSpace2 VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/MP)
endif()

# ---------------------------------------------------------------------------
# JUCE: a local checkout wins, otherwise CMake fetches the pinned release.
# ---------------------------------------------------------------------------
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)

    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.10
        GIT_SHALLOW TRUE)

    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# Plugin
# ---------------------------------------------------------------------------
juce_add_plugin(VocaMaxSpace2
    COMPANY_NAME "Mark33s Beats Lab"
    BUNDLE_ID "com.mark33sbeatslab.vocamaxspace2"
    PLUGIN_MANUFACTURER_CODE Mark
    PLUGIN_CODE VMS2
    PRODUCT_NAME "VocaMax Space 2"
    DESCRIPTION "Vocal space, clarity and tone control for any DAW"
    VERSION "2.0.0"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx
    FORMATS VST3 Standalone)

target_sources(VocaMaxSpace2 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(VocaMaxSpace2 PRIVATE Source)

target_compile_definitions(VocaMaxSpace2 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(VocaMaxSpace2
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if (WIN32)
    set_target_properties(VocaMaxSpace2 PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# ---------------------------------------------------------------------------
# Headless DSP checks (no JUCE required)
# ---------------------------------------------------------------------------
option(VOCAMAX_BUILD_DSP_TESTS "Build the headless VocaMax Space 2 DSP checks" OFF)

if (VOCAMAX_BUILD_DSP_TESTS)
    enable_testing()

    add_executable(VocaMaxDSPTests tests/dsp_test.cpp)
    target_include_directories(VocaMaxDSPTests PRIVATE Source)
    set_target_properties(VocaMaxDSPTests PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON)

    add_test(NAME VocaMaxDSPTests COMMAND VocaMaxDSPTests)
endif()

# VocaMax Space 2

**By Mark33s Beats Lab**

A glassy royal-blue vocal space processor for Windows 10 and 11. VocaMax Space 2 puts a vocal in its own pocket: **Space** controls how much room sits between the vocal and other vocals / instruments, **Power** is the overall depth, **Low** tightens or removes the bottom, and **High** brightens or removes the top. It runs as a **VST3 plugin and a standalone app**, with a proper Windows installer.

The interface is included as an interactive browser preview in [`demo/`](demo/index.html) so you can see and hear the controls before building the plugin.

---

## Controls

| Control | Range | Default | What it does |
| --- | --- | --- | --- |
| **Power** (centre) | 0 – 100 % | 100 % | Overall control. Scales the whole chain. At 0 % the signal is a true, click-free bypass. |
| **Space** | 0 – 100 % | 65 % | Vocal separation: less space is tight and forward, more space opens the stereo image. Bass stays centred. |
| **Low Frequency** (left) | 0 – 100 % | 45 % | Tighten / remove: 24 dB/oct low cut from 20 Hz up to ~240 Hz plus a small low-shelf dip. |
| **High Frequency** (right) | 0 – 100 % | 55 % | Brighten / remove: high shelf at 9 kHz, about −8 dB to +8 dB. |
| **Vocal Focus Mode** | Natural / Clear / Wide / Deep | Clear | Fixed character on top of the four knobs (presence, air, width, low shelf). |

Presets: Default, Natural Vocals, Clear Vocals, Wide Space, Deep Pocket, Tight & Bright, Max Space, Radio Ready.

Tips: drag vertically, hold **Shift** for fine control, double-click a knob to reset it, and use the arrow keys when a knob has focus. Every control is automatable and stored with the project. The top bar has preset browsing, save-to-disk, settings and help.

---

## Build output

`cmake --build` produces:

```
build/VocaMaxSpace2_artefacts/Release/VST3/VocaMax Space 2.vst3
build/VocaMaxSpace2_artefacts/Release/Standalone/VocaMax Space 2.exe
dist/VocaMaxSpace2_Setup.exe                       (after Inno Setup)
```

The installer copies the VST3 to `C:\Program Files\Common Files\VST3\VocaMax Space 2.vst3` and the standalone to `C:\Program Files\VocaMax Space 2\`, adds Start-menu and optional desktop shortcuts, and registers an uninstaller.

> **Note:** this repository contains source, not a prebuilt Windows binary. A `.vst3` must be compiled on Windows (MSVC). Use the included GitHub Actions workflow or the local Windows build script below.

---

## Option A — build with GitHub Actions (no Windows machine needed)

1. Push this folder to a GitHub repository.
2. Open **Actions → Build Windows VST3 + Installer → Run workflow**.
3. Wait for the green run, then download the **VocaMaxSpace2-Windows-x64** artifact.
4. Run `VocaMaxSpace2_Setup.exe` and follow the installer.

The workflow uses `windows-latest`, Visual Studio 2022, CMake, JUCE **8.0.10** (downloaded automatically), runs the headless DSP checks, builds the VST3 and standalone, and compiles the Inno Setup installer. No secrets are required.

---

## Option B — build on Windows

Requirements:

- Windows 10 or 11, 64-bit
- **Visual Studio 2022** with *Desktop development with C++* (MSVC x64 toolchain)
- **CMake 3.22+**
- **Inno Setup 6** (optional, only for the setup `.exe`): <https://jrsoftware.org/isdl.php>
- Git (CMake fetches JUCE)

From the project root:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-windows.ps1
```

The script configures, builds, runs the DSP checks and compiles `dist\VocaMaxSpace2_Setup.exe` when Inno Setup is installed.

Manual equivalent:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DVOCOMAX_BUILD_DSP_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\VocaMaxSpace2.iss
```

To point at an existing JUCE checkout instead of downloading it, place it in `JUCE/` at the project root.

---

## Install and open it in your DAW

Run `VocaMaxSpace2_Setup.exe` (administrator). Then rescan plugins in your DAW:

| DAW | Rescan |
| --- | --- |
| FL Studio | Options → Manage plugins → Find installed plugins |
| Ableton Live | Preferences → Plug-ins → Rescan |
| Cubase / Nuendo | Studio → VST Plug-in Manager → Update |
| Studio One | View → Plug-in Manager → Scan |
| REAPER | Preferences → Plug-ins → VST → Re-scan |
| Bitwig Studio | Settings → Plug-ins → Rescan |
| Pro Tools | Use it through a VST3-capable version / wrapper |

Insert **VocaMax Space 2** on a vocal or a vocal bus. Works with stereo or mono tracks. The standalone app is useful for quick auditions without a DAW.

The installer is not code-signed, so Windows SmartScreen may show a warning: choose **More info → Run anyway**. To remove it, use *Settings → Apps → VocaMax Space 2* (removes the plugin and the standalone app).

---

## Interactive web preview

Open `demo/index.html` directly, or serve it:

```bash
python -m http.server 8000 -d demo
```

The preview mirrors the plugin's layout and control mapping, draws the same glass knobs on canvas, and processes real audio in the browser: play the built-in demo loop, load any audio file, or use a microphone (headphones recommended). It is a design/preview companion — the VST3 is the deliverable for DAWs.

---

## The DSP

Signal path, per channel:

```
low shelf (130 Hz) → 24 dB/oct high pass → 3.2 kHz presence → 9 kHz air shelf
      → mid / side matrix (Space + vocal focus) → out
```

Details:

- **Space** maps to a stereo width multiplier from 0.15× (tight, almost mono) to 2.0× (wide), scaled by Power and the focus mode. The side signal is high-passed (60–220 Hz) so the bass stays centred.
- **Low** raises the high-pass cutoff from 20 Hz to ~240 Hz on a musical exponential curve and adds a −3.5 dB low shelf. It never clicks: cutoff changes are re-interpolated every 32 samples.
- **High** is a 9 kHz high shelf from −8 dB to +8 dB around the centre position.
- **Power** scales the filter depth, the width and the final bypass crossfade. At exactly 0 % the output is the untouched input (verified by tests).
- **Focus modes** add small fixed offsets (presence, air, low shelf, width). `Clear` adds +2.5 dB presence and +1 dB air; `Wide` adds width; `Deep` adds low shelf and a darker top.
- Minimum-phase filters, **zero added latency**, all parameters smoothed (~20 ms), no allocation in `processBlock`, mono and stereo buses supported.

Headless checks live in `tests/dsp_test.cpp` (14 assertions: transparency at Power 0, stability at all sample rates and extremes, width behaviour, low/high tone behaviour, focus modes, click smoothing, mono safety).

Run them without JUCE:

```bash
g++ -std=c++17 -O2 -Wall -Wextra tests/dsp_test.cpp -o dsp_test
./dsp_test
```

---

## Project layout

```
CMakeLists.txt                 JUCE plugin, VST3 + standalone, optional DSP tests
Source/PluginProcessor.*       parameters, presets, state, buses, metering
Source/PluginEditor.*          custom-drawn glass GUI (knobs, meters, focus buttons)
Source/dsp/VocaMaxDSP.h        framework-free DSP engine
tests/dsp_test.cpp             headless DSP checks
installer/VocaMaxSpace2.iss    Inno Setup installer (VST3 + standalone)
scripts/build-windows.ps1      local Windows build
.github/workflows/build-windows.yml  CI build + installer + artifacts
demo/                          interactive browser preview of the interface
```

---

## Verified in this repository

- **DSP:** all 14 headless checks pass with g++ 13 / C++17 (`ALL CHECKS PASSED`).
- **Web preview:** JavaScript passes `node --check`; served and checked over HTTP.
- **Windows plugin build:** not compiled in this Linux workspace. The JUCE/CMake/MSVC build and installer are run by the included Windows workflow or the local script; see the CI badge/log after the first push.

---

## Troubleshooting

- **Plugin missing in the DAW:** confirm `C:\Program Files\Common Files\VST3\VocaMax Space 2.vst3` exists, rescan, and make sure the DAW is running 64-bit.
- **Installer says a file is in use:** close the DAW and the standalone app, then retry.
- **Build cannot find Visual Studio:** run from an *x64 Native Tools Command Prompt for VS 2022*, or install the C++ workload.
- **JUCE download fails:** check network access, or clone JUCE 8.x into `JUCE/` and reconfigure.
- **Microphone in the web preview does not start:** microphone access needs HTTPS or localhost, and browser permission. Use the demo loop or load a file instead.

VST2 is intentionally not built: Steinberg discontinued the VST2 SDK. VST3 is supported by current versions of all major DAWs.

---

**VocaMax Space 2** · By Mark33s Beats Lab · Windows 10 & 11 · 64-bit VST3

build/
dist/
JUCE/
.vs/
*.user
*.suo
*.sln
CMakeUserPresets.json
node_modules/
.DS_Store
