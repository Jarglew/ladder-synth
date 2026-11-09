#!/bin/bash
# Builds Ladder Synth and Layer Stack on a Mac, installs the Audio Units for Logic Pro,
# and checks them with Apple's auval.
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
cmake --build build --config Release --target \
    LadderSynth_AU LadderSynth_VST3 LadderSynth_Standalone \
    LayerStack_AU LayerStack_VST3 LayerStack_Standalone

echo "==> Refreshing the Audio Unit cache"
killall -9 AudioComponentRegistrar 2>/dev/null || true

echo "==> Validating the Audio Units with Apple's auval"
failed=0
for code in Ldr1 Lst1; do
    if auval -v aumu "$code" Jrgl | tail -n 3; then
        echo
    else
        failed=1
    fi
done

if [ "$failed" -eq 0 ]; then
    echo "Done. Both synths are installed in ~/Library/Audio/Plug-Ins/Components."
    echo "In Logic Pro: create a Software Instrument track, then pick"
    echo "Instrument > AU Instruments > Jarglew > Ladder Synth or Layer Stack."
else
    echo "auval reported a problem; see the output above."
    exit 1
fi
