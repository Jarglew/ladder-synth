#!/bin/bash
# Builds Ladder Synth on a Mac, installs the Audio Unit for Logic Pro, and checks it.
# Usage (from the repo folder):  ./scripts/build-mac.sh
set -euo pipefail

cd "$(dirname "$0")/.."

if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake is not installed."
    echo "Install it with Homebrew:   brew install cmake"
    echo "or download CMake.app from https://cmake.org/download/ and run:"
    echo "   sudo \"/Applications/CMake.app/Contents/bin/cmake-gui\" --install"
    exit 1
fi

if ! xcode-select -p >/dev/null 2>&1; then
    echo "Xcode command-line tools are missing. Run:  xcode-select --install"
    exit 1
fi

echo "==> Configuring (the first run downloads JUCE, about 100 MB)"
cmake -S . -B build -G Xcode

echo "==> Building (a few minutes the first time)"
cmake --build build --config Release --target LadderSynth_AU LadderSynth_VST3 LadderSynth_Standalone

echo "==> Refreshing the Audio Unit cache"
killall -9 AudioComponentRegistrar 2>/dev/null || true

echo "==> Validating the Audio Unit with Apple's auval"
if auval -v aumu Ldr1 Jrgl | tail -n 5; then
    echo
    echo "Done. Ladder Synth is installed in ~/Library/Audio/Plug-Ins/Components."
    echo "In Logic Pro: create a Software Instrument track, then pick"
    echo "Instrument > AU Instruments > Jarglew > Ladder Synth."
else
    echo "auval reported a problem; see the output above."
    exit 1
fi
