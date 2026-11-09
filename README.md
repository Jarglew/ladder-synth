# Ladder Synth

A Minimoog-style monophonic/polyphonic subtractive synthesizer, built with
[JUCE](https://juce.com). It builds as an **Audio Unit for Logic Pro**, plus
VST3 and a standalone app.

"Moog" and "Minimoog" are trademarks of Moog Music. This is an independent
instrument inspired by the classic design; it uses none of their names,
artwork or code.

## What's in it

- **Three oscillators**: seven waveforms (triangle, tri-saw, reverse saw, saw,
  square, wide and narrow pulse), footage ranges Lo/32'/16'/8'/4'/2',
  ±7 semitone tuning on oscillators 2 and 3. Oscillator 3 can drop its
  keyboard tracking and run in the Lo range as an LFO.
- **Mixer** with white or pink noise.
- **24 dB/octave transistor-ladder filter**: cutoff, emphasis (self-oscillates
  near the top), contour amount, drive and keyboard tracking (off, 1/3, 2/3, full).
- **Filter and loudness envelopes** (ADSR, analog-style curves).
- **Mono or poly** (8 voices): last-note priority, legato and glide in mono.
- **Modulation**: oscillator 3 or noise to the oscillators and/or the filter,
  depth from the mod wheel plus a Mod Amount knob.
- Pitch bend (adjustable range), sustain pedal, velocity to loudness and filter.
- 10 factory presets, shown in Logic's preset menu.
- Resizable panel; double-click any knob to reset it.

## Building on a Mac (for Logic Pro)

You need Xcode (free from the App Store) and CMake (`brew install cmake`).

```bash
git clone https://github.com/Jarglew/ladder-synth.git
cd ladder-synth
./scripts/build-mac.sh
```

The script downloads JUCE, builds a universal (Apple silicon + Intel) Audio
Unit, installs it to `~/Library/Audio/Plug-Ins/Components`, and runs Apple's
`auval` check. Then in Logic Pro: create a **Software Instrument** track and
choose **Instrument → AU Instruments → Jarglew → Ladder Synth**. If Logic was
open during the build, quit and reopen it, or rescan in
**Logic Pro → Settings → Plug-in Manager**.

To work on it in Xcode instead: `cmake -S . -B build -G Xcode`, then open
`build/LadderSynth.xcodeproj`.

## Tests

The sound engine is plain C++ with its own tests (pitch accuracy, filter,
resonance, mono/poly voice handling, glide, sustain, voice stealing):

```bash
cmake -S . -B build -DLADDER_BUILD_PLUGIN=OFF && cmake --build build && ctest --test-dir build
```

`-DLADDER_BUILD_PLUGIN_TESTS=ON` adds a smoke test that drives the real plugin
processor and editor without a host: several sample rates and block sizes,
every preset, random parameter sweeps, save/restore, and a PNG of the panel.

## Layout

| Path | What |
| --- | --- |
| `Source/dsp/` | The sound engine: oscillators, ladder filter, envelopes, voices. No JUCE. |
| `Source/Parameters.*` | Every automatable parameter. |
| `Source/Presets.*` | Factory presets. |
| `Source/PluginProcessor.*` | The plugin: MIDI handling, state saving, presets. |
| `Source/PluginEditor.*` | The panel. |
| `tests/` | Engine tests and the plugin smoke test. |

## Licence

JUCE is dual-licensed (AGPLv3 or a commercial licence). Building this for your
own use is fine. If you ever distribute the plugin, check JUCE's licence terms
first: distributing under the free licence means releasing this source code
under the AGPLv3.
