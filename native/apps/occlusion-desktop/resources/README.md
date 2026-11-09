# Built-in teaching arches v1

Original procedural teaching material, distributed under the repository license.
Each arch consists of 14 separated, closed, outward-oriented ellipsoidal shells.
These are schematic crowns, not scanned teeth, anatomical reconstructions, or clinical data.
They contain no patient information. Coordinates are meters.

The lower mesh compensates the existing engine's +0.16 m closed-mandible translation.
At 8 mm opening the surfaces are separated by 8 mm; at zero opening the opposing poles meet.
The model does not simulate temporomandibular rotation or a physiological occlusal surface.

Regenerate deterministically with `python3 scripts/generate-desktop-demo.py` from the repository root.
Qt resources embed both OBJ files into the executable. The UI copies them into its own temporary
directory and uses the same file/topology validation and evaluator as an imported mesh pair.
Exports identify this material as `desktop-teaching-arches-v1`; the numerical v1 schema is unchanged.
The packaged smoke verifies startup, interaction, sweep playback, reset, and export without
requiring these OBJ files next to the executable.
