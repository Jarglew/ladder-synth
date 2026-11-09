#!/bin/bash
# Builds Ladder Synth and Layer Stack on a Mac, installs the Audio Units for Logic Pro,
# and checks them with Apple's auval.
# Usage (from the repo folder):  ./scripts/build-mac.sh
set -euo pipefail

cd "$(dirname "$0")/.."

# CMake: use one that's installed, otherwise download a private copy into .tools/
if ! command -v cmake >/dev/null 2>&1; then
    CMAKE_VERSION=3.30.5
    CMAKE_DIR="$PWD/.tools/cmake-$CMAKE_VERSION-macos-universal"
    if [ ! -x "$CMAKE_DIR/CMake.app/Contents/bin/cmake" ]; then
        echo "==> CMake isn't installed, so downloading a copy just for this project (about 50 MB, one time only)"
        mkdir -p .tools
        curl -fL --progress-bar \
            "https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION-macos-universal.tar.gz" \
            | tar -xz -C .tools
    fi
    export PATH="$CMAKE_DIR/CMake.app/Contents/bin:$PATH"
fi

# The Xcode build needs the full Xcode app, not just the command-line tools.
if ! xcodebuild -version >/dev/null 2>&1; then
    echo "Xcode isn't set up for building."
    echo "If Xcode is installed in Applications, run this once, then run this script again:"
    echo "   sudo xcode-select -s /Applications/Xcode.app/Contents/Developer"
    echo "Otherwise install Xcode from the App Store first."
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
