# Ladder Synth & Layer Stack

Two synthesizer plugins built with [JUCE](https://juce.com). Both build as
**Audio Units for Logic Pro**, plus VST3 and standalone apps.

- **Ladder Synth**: a Minimoog-style monophonic/polyphonic subtractive synth.
- **Layer Stack**: a layered preset synth in the spirit of big "rompler"
  workstations: four stacked layers, arpeggiator, trance gate, effects chain
  and a preset browser. Its layers are built-in synth oscillators or your
  own samples.

"Moog", "Minimoog" and "Nexus" are trademarks of their owners. These are
independent instruments inspired by those designs; they use none of their
names, artwork, samples or code.

# Ladder Synth

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


# Layer Stack

## What's in it

- **Four layers per patch**, each with its own on/off, level and pan, and its
  own source: saw, square, triangle, sine, **supersaw** (seven detuned saws
  with stereo spread), noise, or a **sample** you load (WAV, AIFF, FLAC, Ogg;
  set its root note, loop on or off). Octave, semitone and fine tuning per layer.
- **Per-layer filter**: 24 dB transistor ladder, or 12 dB low-pass, high-pass
  or band-pass, with envelope amount, key tracking and its own ADSR.
- **Per-layer amp envelope** and velocity sensitivity.
- **Arpeggiator** locked to Logic's tempo: up, down, up/down, down/up, random,
  as played, chord; 1/4 to 1/32 including triplets; 1 to 4 octaves; gate,
  swing, latch; a 16-step pattern with per-step on/off and velocity.
- **Trance gate**: 16-step on/off pattern, tempo-synced, with depth, attack
  and release.
- **Effects chain**, reorderable: distortion, chorus, phaser, tempo-synced
  (ping-pong) delay, reverb. A soft limiter at the end keeps output under 0 dBFS.
- **Modulation**: LFO 1 to every filter, LFO 2 to pitch (vibrato), mod wheel
  to vibrato; mono or 16-voice poly with glide; pitch bend; sustain pedal.
- **Preset browser**: 26 factory presets in eight categories (lead, pad,
  gated, bass, pluck, keys, arp, FX), search, and your own presets saved to
  `~/Music/Layer Stack/Presets`. Factory presets also show in Logic's preset menu.

Layer Stack ships with synth sounds only. To use samples, click a layer,
press **Load...** in the Sample box, and pick an audio file. The patch
remembers the file's location, so keep your samples where they are.

# Building and testing

## On a Mac (for Logic Pro)

You need Xcode (free from the App Store) and CMake (`brew install cmake`).

```bash
git clone https://github.com/Jarglew/ladder-synth.git
cd ladder-synth
./scripts/build-mac.sh
```

The script downloads JUCE, builds universal (Apple silicon + Intel) Audio
Units of both synths, installs them to `~/Library/Audio/Plug-Ins/Components`,
and runs Apple's `auval` check on each. Then in Logic Pro: create a
**Software Instrument** track and choose
**Instrument → AU Instruments → Jarglew → Ladder Synth** (or **Layer Stack**). If Logic was
open during the build, quit and reopen it, or rescan in
**Logic Pro → Settings → Plug-in Manager**.

To work on it in Xcode instead: `cmake -S . -B build -G Xcode`, then open
`build/LadderSynth.xcodeproj`.

## Tests

Both sound engines are plain C++ with their own tests: pitch accuracy,
filters, resonance, mono/poly voice handling, glide, sustain, voice stealing,
and for Layer Stack the arpeggiator's timing, note order, host sync, latch and
patterns, the trance gate, stereo layers and sample playback:

```bash
cmake -S . -B build -DLADDER_BUILD_PLUGIN=OFF && cmake --build build && ctest --test-dir build
```

`-DLADDER_BUILD_PLUGIN_TESTS=ON` adds a smoke test for each plugin that
drives the real processor and editor without a host: several sample rates and
block sizes, every preset, random parameter sweeps, save/restore, and PNGs of
the panel. Layer Stack's also loads a WAV into a layer and saves and reloads a
user preset.

## Layout

| Path | What |
| --- | --- |
| `Source/dsp/` | Ladder Synth's engine: oscillators, ladder filter, envelopes, voices. No JUCE. Layer Stack reuses these. |
| `Source/` | Ladder Synth's plugin, parameters, presets and panel. |
| `Stack/Source/engine/` | Layer Stack's engine: layers, supersaw, samples, arpeggiator, trance gate, LFOs. No JUCE. |
| `Stack/Source/fx/` | Layer Stack's effects chain. |
| `Stack/Source/` | Layer Stack's plugin, parameters, presets and editor. |
| `tests/`, `Stack/tests/` | Engine tests and plugin smoke tests. |

## Licence

JUCE is dual-licensed (AGPLv3 or a commercial licence). Building this for your
own use is fine. If you ever distribute the plugin, check JUCE's licence terms
first: distributing under the free licence means releasing this source code
under the AGPLv3. Samples you load into Layer Stack stay under their own licences.
