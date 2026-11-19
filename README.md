# Arc EQ

A parametric EQ plugin (AU + VST3) for macOS on Apple Silicon, built with [JUCE](https://juce.com).

**Status: phase 1.** This first version exists to check that the plugin builds and loads in your DAW. It has:

- 4 bell bands on by default, out of 10 band slots
- A live spectrum of the output
- A summed EQ curve with draggable band nodes
- A bottom bar with Freq, Gain and Q for the selected band, plus Output gain

## Controls

| Action | What it does |
| --- | --- |
| Drag a node | Changes frequency and gain |
| Shift-drag a node | Changes frequency only |
| Scroll over a node | Changes Q |
| Double-click a node | Turns that band off |
| Double-click empty space | Turns on a free band at that spot |

## Build on macOS

### 1. Install the tools (one time)

```bash
xcode-select --install          # Apple's compilers
brew install cmake              # needs Homebrew: https://brew.sh
```

### 2. Build

```bash
git clone https://github.com/tiryqthomas/eq-plugin.git
cd eq-plugin
cmake -B build -G Xcode
cmake --build build --config Release
```

The first configure downloads JUCE 8, which takes a few minutes. The build copies the plugins into place automatically:

- AU: `~/Library/Audio/Plug-Ins/Components/Arc EQ.component`
- VST3: `~/Library/Audio/Plug-Ins/VST3/Arc EQ.vst3`
- Standalone app: `build/ArcEQ_artefacts/Release/Standalone/Arc EQ.app`

If you already have JUCE on disk, skip the download with `-DJUCE_DIR=/path/to/JUCE`.

### 3. Load it in your DAW

- **Logic Pro / GarageBand (AU):** check it passes Apple's validator with `auval -v aufx Arq1 Tity`. If Logic doesn't show it, open Logic's Plug-in Manager and rescan, or run `killall -9 AudioComponentRegistrar`.
- **Ableton Live / others (VST3):** rescan plug-ins in the DAW's preferences.

The plugin shows up under the manufacturer **Ti**.

## Project layout

```
CMakeLists.txt            Build setup, plugin name and IDs
Source/
  Parameters.h            Band count, ranges and parameter IDs
  PluginProcessor.*       Audio processing (bell filters, output gain)
  SpectrumAnalyzer.*      Lock-free audio-to-UI hand-off, plus the FFT
  EqCurveView.*           Spectrum, EQ curve and draggable nodes
  PluginEditor.*          Window layout, bottom bar and knob style
  Theme.h                 All colours in one place
```

## Roadmap

These are planned and will be phased in over time:

- Add and delete bands (up to 10), plus shelf, cut and notch filter types
- Dynamic EQ: per-band threshold, attack and release, sidechain, spectral dynamics
- Reference track: load a song, compare its spectrum with your mix, and EQ-match
- Per-band mid/side and left/right processing
- Zero-latency, linear-phase and natural-phase modes
- Gentle and Warm character modes
- A/B comparison
