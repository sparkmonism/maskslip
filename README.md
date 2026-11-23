# MaskSlip

Songcraft Audio. Shows which frequencies are being masked between tracks (AU / VST3 / Standalone), built with JUCE.

The audio is never changed: MaskSlip is a window, not an effect.

## How it works

Put a MaskSlip on each track you want to compare. Every instance analyses its own track and shares
the result with the other instances running in the same host, so any one of them can show how the
tracks interact. Name each one in the TRACK NAME box (Bass, Vocal, Guitar...).

Open any instance:

- **WHAT MASKS ME** (default): the coral line is the masking threshold created by all the other
  tracks together. Where your track (cyan) sits underneath it, that part is masked: it is shaded coral
  and listed in the report with the track responsible.
- **WHAT I MASK**: the reverse. The line shows how far this track's energy reaches into the
  others, and the shading marks where it covers them.

Hover the graph for the frequency, the amount, and the track involved. Click a track in the TRACKS list
to leave it out of the comparison.

The strip under the graph is a heat map of masking depth across the spectrum.

## Controls

- **Sensitivity**: how readily masking is flagged. Higher means a more cautious (more masking) reading.
- **Min depth**: smallest masking, in dB, that gets shaded and reported.
- **Speed**: how quickly the analysis follows the audio (Fast / Medium / Slow).

## Sidechain input

If your host keeps plugins separate (or you want a track from outside the project), enable the plugin's
sidechain input and send any track to it. It appears in the list as "Sidechain".

## Build (macOS)

Requires Xcode command line tools and CMake (`brew install cmake`). From inside this folder,
one command at a time:

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j 8
    killall -9 AudioComponentRegistrar
    auval -v aufx Mksl Sngc

## Notes and limits

- Instances find each other only inside the same host process and the same plugin format. Mixing AU on
  one track and VST3 on another will not link them. Some hosts run plugins in separate sandboxes; use
  the sidechain input there.
- A track is "on" while its plugin is processing audio. A muted or idle track drops out after
  about a second.
- The masking model is the standard critical-band (Bark) spread used in perceptual coding, with a
  tonality offset set by Sensitivity. It is a guide for where tracks compete, not a measurement of
  what you will hear: trust your ears, then use it to find where to cut.
- The analysis is of the mono sum of each track.
